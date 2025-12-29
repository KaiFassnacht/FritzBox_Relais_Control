#include <Arduino.h>
#include <ETH.h>
#include <WiFiUdp.h>
#include <MD5Builder.h>
#include "sip.h"

Sip::Sip(char *pBuf, size_t lBuf) {
    pbuf = pBuf;
    lbuf = lBuf;
    pDialNr = "";
    pDialDesc = "";
    audioport[0] = '\0';
    iRingTime = 0;
    callid = 0;
    tagid = 0;
    lastDtmfDigit = 0;
}

void Sip::Init(const char *SipIp, int SipPort, const char *MyIp, int MyPort, const char *SipUser, const char *SipPassWd) {
    udp.begin(SipPort);
    pSipIp = SipIp;
    iSipPort = SipPort;
    pSipUser = SipUser;
    pSipPassWd = SipPassWd;
    pMyIp = MyIp;
    iMyPort = MyPort;
    iRingTime = 0;
    Serial.printf("DEBUG| SIP Init: Port %d, Server: %s, MyIP: %s\n", SipPort, SipIp, MyIp);
}

void Sip::HandleUdpPacket() {
    static char caSipIn[2048];
    int packetSize = udp.parsePacket();
    
    if (packetSize <= 0) return;

    int len = udp.read(caSipIn, sizeof(caSipIn) - 1);
    caSipIn[len] = 0;
    char *p = caSipIn;

    Serial.println("\n>>> SIP EMPFANGEN:");
    Serial.println(p);
    Serial.println("-------------------");

    // --- 1. Antwort auf Unauthorized (401) ---
    if (strstr(p, "SIP/2.0 401 Unauthorized") == p) {
        if (strstr(p, "CSeq: 1 REGISTER") || strstr(p, "CSeq: 2 REGISTER") || strstr(p, "CSeq: 100 REGISTER")) {
            Serial.println("DEBUG| 401 für REGISTER -> Sende Auth...");
            Register(p); 
        } else {
            Serial.println("DEBUG| 401 für INVITE -> Sende ACK & Auth...");
            Ack(p);
            Invite(p);
        }
    }
    
    // --- 2. Antwort auf OK (200) ---
    else if (strstr(p, "SIP/2.0 200") == p) {
        Serial.println("DEBUG| 200 OK erhalten.");
        if (strstr(p, "CSeq: 1 INVITE") || strstr(p, "CSeq: 2 INVITE")) {
            Ack(p);
        }
    }

    // --- 3. Eingehender Anruf (INVITE) ---
    else if (strstr(p, "INVITE sip:") == p) {
        Serial.println("DEBUG| EINGEHENDER ANRUF ERKANNT!");
        
        ParseParameter(caCallIdIn, 128, "Call-ID: ", p, '\r');
        if (strlen(caCallIdIn) == 0) ParseParameter(caCallIdIn, 128, "Call-ID: ", p, '\n');

        // From Header extrahieren
        char* f = strstr(p, "From: ");
        if (f) {
            char* fe = strpbrk(f, "\r\n");
            int flen = fe - f - 6;
            if (flen > 127) flen = 127;
            strncpy(caFromIn, f + 6, flen);
            caFromIn[flen] = 0;
        }

        // To Header extrahieren
        char* t = strstr(p, "To: ");
        if (t) {
            char* te = strpbrk(t, "\r\n");
            int tlen = te - t - 4;
            if (tlen > 127) tlen = 127;
            strncpy(caToIn, t + 4, tlen);
            caToIn[tlen] = 0;
        }

        iRingTime = millis();
        Ok(p); // Den Anruf annehmen
    }

    // --- 4. DTMF Töne (INFO) ---
    else if (strstr(p, "INFO sip:") == p) {
        Serial.println("DEBUG| DTMF INFO empfangen!");
        
        // Suche nach "Signal=" im Body der Nachricht
        char* s = strstr(p, "Signal=");
        if (s) {
            // Pointer auf das Zeichen nach "Signal=" setzen
            char* digitPtr = s + 7;
            // Eventuelle Leerzeichen überspringen
            while (*digitPtr == ' ') digitPtr++;
            
            lastDtmfDigit = *digitPtr; 
            Serial.printf("DEBUG| Taste erkannt: %c\n", lastDtmfDigit);
        }
        
        // Jede INFO-Anfrage muss zwingend mit 200 OK beantwortet werden
        Ok(p);
    }

    // --- 5. Aufgelegt (BYE) ---
    else if (strstr(p, "BYE sip:") == p) {
        Serial.println("DEBUG| Aufgelegt.");
        Ok(p);
        iRingTime = 0;
        lastDtmfDigit = 0; // Reset der DTMF Variable
    }
}

// --- HIER SIND DIE FEHLENDEN METHODEN FÜR DEN LINKER ---

bool Sip::Dial(const char *DialNr, const char *DialDesc, int MaxDialSec) {
    if (iRingTime) return false; // Bereits beschäftigt
    
    Serial.printf("DEBUG| Dialing: %s (%s)\n", DialNr, DialDesc);
    pDialNr = DialNr;
    pDialDesc = DialDesc;
    
    // Neue IDs für diesen Anruf generieren
    callid = Random();
    tagid = Random();
    branchid = Random();
    
    iRingTime = millis();
    iMaxTime = MaxDialSec * 1000;
    
    Invite(NULL); // Erster Invite ohne Auth
    return true;
}

void Sip::Invite(const char *pIn) {
    pbuf[0] = 0;
    int cseq = (pIn) ? 2 : 1;

    AddSipLine("INVITE sip:%s@%s SIP/2.0", pDialNr, pSipIp);
    AddSipLine("Via: SIP/2.0/UDP %s:%i;branch=z9hG4bK%010u", pMyIp, iMyPort, branchid);
    AddSipLine("From: \"%s\" <sip:%s@%s>;tag=%010u", pDialDesc, pSipUser, pSipIp, tagid);
    AddSipLine("To: <sip:%s@%s>", pDialNr, pSipIp);
    AddSipLine("Call-ID: %010u@%s", callid, pMyIp);
    AddSipLine("CSeq: %i INVITE", cseq);
    AddSipLine("Contact: <sip:%s@%s:%i>", pSipUser, pMyIp, iMyPort);
    AddSipLine("Max-Forwards: 70");
    
    if (pIn) {
        char caRealm[128], caNonce[128];
        if (ParseParameter(caRealm, 128, " realm=\"", pIn) && ParseParameter(caNonce, 128, " nonce=\"", pIn)) {
            char ha1Hex[33], ha2Hex[33], haResp[33], pTemp[512];
            snprintf(pTemp, sizeof(pTemp), "%s:%s:%s", pSipUser, caRealm, pSipPassWd);
            MakeMd5Digest(ha1Hex, pTemp);
            snprintf(pTemp, sizeof(pTemp), "INVITE:sip:%s@%s", pDialNr, pSipIp);
            MakeMd5Digest(ha2Hex, pTemp);
            snprintf(pTemp, sizeof(pTemp), "%s:%s:%s", ha1Hex, caNonce, ha2Hex);
            MakeMd5Digest(haResp, pTemp);
            AddSipLine("Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"sip:%s@%s\", response=\"%s\"", 
                       pSipUser, caRealm, caNonce, pDialNr, pSipIp, haResp);
        }
    }

    AddSipLine("Content-Type: application/sdp");
    AddSipLine("Content-Length: 120");
    AddSipLine("");
    AddSipLine("v=0\r\no=- 0 0 IN IP4 %s\r\ns=-\r\nc=IN IP4 %s\r\nt=0 0\r\nm=audio 10000 RTP/AVP 8\r\na=rtpmap:8 PCMA/8000", pMyIp, pMyIp);
    SendUdp();
}

// --- RESTLICHE METHODEN ---

void Sip::Register() {
    Register(NULL);
}

void Sip::Register(const char *pIn) {
    pbuf[0] = 0;
    int cseq = (pIn) ? GrepInteger(pIn, "CSeq: ") + 1 : 1;

    if (!pIn) {
        callid = Random();
        tagid = Random();
    }

    AddSipLine("REGISTER sip:%s SIP/2.0", pSipIp);
    AddSipLine("Via: SIP/2.0/UDP %s:%i;branch=z9hG4bK%010u", pMyIp, iMyPort, Random());
    AddSipLine("From: <sip:%s@%s>;tag=%010u", pSipUser, pSipIp, tagid);
    AddSipLine("To: <sip:%s@%s>", pSipUser, pSipIp);
    AddSipLine("Call-ID: %010u@%s", callid, pMyIp);
    AddSipLine("CSeq: %i REGISTER", cseq);
    AddSipLine("Contact: <sip:%s@%s:%i>", pSipUser, pMyIp, iMyPort);
    AddSipLine("Max-Forwards: 70");
    AddSipLine("Expires: 3600");

    if (pIn) {
        char caRealm[128], caNonce[128];
        if (ParseParameter(caRealm, 128, " realm=\"", pIn) && ParseParameter(caNonce, 128, " nonce=\"", pIn)) {
            char ha1Hex[33], ha2Hex[33], haResp[33], pTemp[512];
            snprintf(pTemp, sizeof(pTemp), "%s:%s:%s", pSipUser, caRealm, pSipPassWd);
            MakeMd5Digest(ha1Hex, pTemp);
            snprintf(pTemp, sizeof(pTemp), "REGISTER:sip:%s", pSipIp);
            MakeMd5Digest(ha2Hex, pTemp);
            snprintf(pTemp, sizeof(pTemp), "%s:%s:%s", ha1Hex, caNonce, ha2Hex);
            MakeMd5Digest(haResp, pTemp);
            AddSipLine("Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"sip:%s\", response=\"%s\"", 
                       pSipUser, caRealm, caNonce, pSipIp, haResp);
        }
    }
    AddSipLine("Content-Length: 0");
    AddSipLine("");
    SendUdp();
}

void Sip::Ack(const char *pIn) {
    char caTo[128];
    if (!ParseParameter(caTo, 128, "To: ", pIn, '\r')) ParseParameter(caTo, 128, "To: ", pIn, '\n');
    pbuf[0] = 0;
    AddSipLine("ACK %s SIP/2.0", caTo);
    AddCopySipLine(pIn, "Call-ID: ");
    AddSipLine("CSeq: %i ACK", GrepInteger(pIn, "CSeq: "));
    AddCopySipLine(pIn, "From: ");
    AddCopySipLine(pIn, "Via: ");
    AddSipLine("To: %s", caTo);
    AddSipLine("Content-Length: 0");
    AddSipLine("");
    SendUdp();
}

void Sip::Ok(const char *pIn) {
    pbuf[0] = 0;
    AddSipLine("SIP/2.0 200 OK");
    AddCopySipLine(pIn, "Call-ID: ");
    AddCopySipLine(pIn, "CSeq: ");
    AddCopySipLine(pIn, "From: ");
    AddCopySipLine(pIn, "Via: ");
    AddCopySipLine(pIn, "To: ");
    AddSipLine("Content-Length: 0");
    AddSipLine("");
    SendUdp();
}

void Sip::AddSipLine(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    char line[512];
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    strcat(pbuf, line);
    strcat(pbuf, "\r\n");
}

bool Sip::ParseParameter(char *dest, int destlen, const char *name, const char *line, char cq) {
    const char *r = strstr(line, name);
    if (!r) return false;
    r += strlen(name);
    const char *qp = strpbrk(r, "\"\r\n>"); 
    int l = qp ? (qp - r) : strlen(r);
    if (l >= destlen) l = destlen - 1;
    strncpy(dest, r, l);
    dest[l] = 0;
    return true;
}

bool Sip::AddCopySipLine(const char *p, const char *psearch) {
    const char *pa = strstr(p, psearch);
    if (!pa) return false;
    const char *pe = strpbrk(pa, "\r\n");
    if (!pe) return false;
    strncat(pbuf, pa, pe - pa);
    strcat(pbuf, "\r\n");
    return true;
}

int Sip::GrepInteger(const char *p, const char *psearch) {
    const char *pc = strstr(p, psearch);
    return pc ? atoi(pc + strlen(psearch)) : 0;
}

int Sip::SendUdp() {
    Serial.println("<<< SIP SENDEN:");
    Serial.println(pbuf);
    Serial.println("-------------------");
    udp.beginPacket(pSipIp, iSipPort);
    udp.write((uint8_t *)pbuf, strlen(pbuf));
    return udp.endPacket();
}

uint32_t Sip::Random() { return esp_random(); }
void Sip::MakeMd5Digest(char *pOutHex33, char *pIn) {
    MD5Builder aMd5;
    aMd5.begin();
    aMd5.add(pIn);
    aMd5.calculate();
    aMd5.getChars(pOutHex33);
}

void Sip::Cancel(int cseq) {
    pbuf[0] = 0;
    AddSipLine("CANCEL sip:%s@%s SIP/2.0", pDialNr, pSipIp);
    AddSipLine("Via: SIP/2.0/UDP %s:%i;branch=z9hG4bK%010u", pMyIp, iMyPort, branchid);
    AddSipLine("From: <sip:%s@%s>;tag=%010u", pSipUser, pSipIp, tagid);
    AddSipLine("To: <sip:%s@%s>", pDialNr, pSipIp);
    AddSipLine("Call-ID: %010u@%s", callid, pMyIp);
    AddSipLine("CSeq: %i CANCEL", cseq);
    AddSipLine("Max-Forwards: 70");
    AddSipLine("Content-Length: 0");
    AddSipLine("");
    SendUdp();
}

void Sip::Bye(int cseq) {
    if (strlen(caCallIdIn) == 0) {
        Serial.println("DEBUG| Bye abgebrochen: Keine Call-ID vorhanden.");
        return;
    }

    pbuf[0] = 0;
    AddSipLine("BYE sip:%s SIP/2.0", pSipIp);
    AddSipLine("Via: SIP/2.0/UDP %s:%i;branch=z9hG4bK%010u", pMyIp, iMyPort, Random());
    
    // WICHTIG: From und To vom INVITE müssen gespiegelt werden
    AddSipLine("From: %s", caToIn); 
    AddSipLine("To: %s", caFromIn);
    AddSipLine("Call-ID: %s", caCallIdIn);
    
    AddSipLine("CSeq: %i BYE", cseq);
    AddSipLine("Max-Forwards: 70");
    AddSipLine("User-Agent: WT32-ETH01");
    AddSipLine("Content-Length: 0");
    AddSipLine("");
    
    SendUdp();
    iRingTime = 0;
    // Wir lassen caCallIdIn[0] = 0; hier weg, damit man zur Not mehrmals senden kann, 
    // falls das erste Paket verloren geht.
}

const char* Sip::GetSIPServerIP() { return pSipIp; }