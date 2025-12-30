/* ====================================================================

   Copyright (c) 2018 Juerge Liegner  All rights reserved.
   modifications by Jens Weißkopf
   modifications by Kai Fassnacht

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.

   3. Neither the name of the author(s) nor the names of any contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.

   ====================================================================*/

#include "sip.h"
#include "settings.h"

Sip::Sip(char *pBuf, size_t lBuf) {
    pbuf = pBuf;
    lbuf = lBuf;
    iRingTime = 0;
    lastDtmfDigit = 0;
}

void Sip::Init(const char *SipIp, int SipPort, const char *MyIp, int MyPort, const char *SipUser, const char *SipPassWd) {
    pSipIp = SipIp;
    iSipPort = SipPort;
    pSipUser = SipUser;
    pSipPassWd = SipPassWd;
    pMyIp = MyIp;
    iMyPort = MyPort;
    
    udp.begin(iSipPort); 
    rtp.begin(SipConfig::RTP_PORT);
}

void Sip::HandleUdpPacket() {
    static char caSipIn[2048];
    int packetSize = udp.parsePacket();
    
    if (packetSize <= 0) return;

    int len = udp.read(caSipIn, sizeof(caSipIn) - 1);
    caSipIn[len] = 0;
    char *p = caSipIn;

    // --- NEUES DEBUGGING ---
    if (strstr(p, "ACK sip:")) {
        Serial.println(">>> DEBUG| ACK empfangen! Die FritzBox hat unser 200 OK akzeptiert.");
        this->isConnected = true; // Setze ein Flag in sip.h (bool isConnected)
    }

    if (strstr(p, "SIP/2.0 481") || strstr(p, "SIP/2.0 408")) {
        Serial.println(">>> DEBUG| FEHLER: FritzBox meldet Timeout oder unbekannten Dialog.");
    }
    // -----------------------

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
        this->isConnected = false; // Reset für neuen Anruf
        Serial.println("DEBUG| EINGEHENDER ANRUF ERKANNT!");
        
        ParseParameter(caCallIdIn, 128, "Call-ID: ", p, '\r');
        if (strlen(caCallIdIn) == 0) ParseParameter(caCallIdIn, 128, "Call-ID: ", p, '\n');

        char bufPort[8];
        if (ParseParameter(bufPort, 8, "m=audio ", p, ' ')) {
            uint16_t port = atoi(bufPort);
            rtp.begin(SipConfig::RTP_PORT); 
            rtp.setTarget(pSipIp, port);
}

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
        
        iLastInCSeq = GrepInteger(p, "CSeq: "); // Speichere die CSeq der Fritzbox
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
    AddSipLine("v=0\r\no=- 0 0 IN IP4 %s\r\ns=-\r\nc=IN IP4 %s\r\nt=0 0\r\nm=audio %d RTP/AVP 8\r\na=rtpmap:8 PCMA/8000", 
               pMyIp, pMyIp, SipConfig::RTP_PORT); 
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
    AddCopySipLine(pIn, "Via: ");
    AddCopySipLine(pIn, "From: ");

    // To-Header mit Tag (Wichtig für Session-Zuordnung)
    char tempTo[128];
    ParseParameter(tempTo, 128, "To: ", pIn, '\r');
    if (!strstr(tempTo, ";tag=")) {
        if (tempTo[strlen(tempTo)-1] == '>') tempTo[strlen(tempTo)-1] = '\0';
        sprintf(caToIn, "%s>;tag=%010u", tempTo, tagid);
    } else {
        strcpy(caToIn, tempTo);
    }
    AddSipLine("To: %s", caToIn);
    AddSipLine("Contact: <sip:%s@%s:%i>", pSipUser, pMyIp, iMyPort);
    
    if (strstr(pIn, "INVITE sip:")) {
        // SDP Body vorbereiten
        char sdp[256];
        snprintf(sdp, sizeof(sdp), 
            "v=0\r\n"
            "o=- 0 0 IN IP4 %s\r\n"
            "s=-\r\n"
            "c=IN IP4 %s\r\n"
            "t=0 0\r\n"
            "m=audio %d RTP/AVP 8\r\n"
            "a=rtpmap:8 PCMA/8000\r\n", 
            pMyIp, pMyIp, SipConfig::RTP_PORT);
            
        AddSipLine("Content-Type: application/sdp");
        AddSipLine("Content-Length: %d", (int)strlen(sdp));
        AddSipLine(""); 
        strcat(pbuf, sdp);
        Serial.println("DEBUG| Sende 200 OK mit SDP (Audio-Handshake)");
    } else {
        AddSipLine("Content-Length: 0");
        AddSipLine("");
        Serial.println("DEBUG| Sende 200 OK (ohne SDP)");
    }

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
    if (strlen(caCallIdIn) == 0) return;

    int useCSeq = (cseq > 0) ? cseq : iLastCSeq + 10;

    pbuf[0] = 0;
    AddSipLine("BYE sip:%s SIP/2.0", pSipIp);
    AddSipLine("Via: SIP/2.0/UDP %s:%i;branch=z9hG4bK%010u", pMyIp, iMyPort, Random());
    
    // Falls caToIn leer geblieben ist (Sicherheitsnetz)
    if (strlen(caToIn) < 5) {
        AddSipLine("From: <sip:%s@%s:%i>;tag=%010u", pSipUser, pMyIp, iMyPort, tagid);
    } else {
        AddSipLine("From: %s", caToIn); 
    }

    AddSipLine("To: %s", caFromIn);
    AddSipLine("Call-ID: %s", caCallIdIn);
    AddSipLine("CSeq: %i BYE", useCSeq);
    AddSipLine("Max-Forwards: 70");
    AddSipLine("Content-Length: 0");
    AddSipLine("");
    
    SendUdp();
    iRingTime = 0;
    caCallIdIn[0] = 0; 
}


const char* Sip::GetSIPServerIP() { return pSipIp; }