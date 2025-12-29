#ifndef SIP_h
#define SIP_h

#include <Arduino.h>
#include <ETH.h>
#include <WiFiUdp.h>

class Sip
{
  private: 
    WiFiUDP     udp;
    char       *pbuf;
    size_t      lbuf;
    char        caRead[256];
    char        caCaller[64];
    char        caCallIdIn[128];
    char        caFromIn[128];
    char        caToIn[128];

    const char *pSipIp;
    int         iSipPort;
    const char *pSipUser;
    const char *pSipPassWd;
    const char *pMyIp;
    int         iMyPort;
    const char *pDialNr;
    const char *pDialDesc;

    uint32_t    callid;
    uint32_t    tagid;
    uint32_t    branchid;

    int         iAuthCnt;
    uint32_t    iRingTime;
    uint32_t    iMaxTime;
    int         iDialRetries;
    int         iLastCSeq;

    // Interne Hilfsmethoden
    void        AddSipLine(const char* constFormat , ... );
    bool        AddCopySipLine(const char *p, const char *psearch);   
    bool        ParseParameter(char *dest, int destlen, const char *name, const char *line, char cq = '\"');
    bool        ParseReturnParams(const char *p);
    int         GrepInteger(const char *p, const char *psearch);
    void        Ack(const char *pIn);
    void        Ok(const char *pIn);
    void        Invite(const char *pIn = 0);
    
    // Interne Überladung für das Antwort-Register (MD5 Auth)
    void        Register(const char *pIn); 

    uint32_t    Millis();
    uint32_t    Random();
    int         SendUdp();
    void        MakeMd5Digest(char *pOutHex33, char *pIn);

    
  public: 
    Sip(char *pBuf, size_t lBuf);
    void        Init(const char *SipIp, int SipPort, const char *MyIp, int MyPort, const char *SipUser, const char *SipPassWd);
    void        HandleUdpPacket();
    bool        Dial(const char *DialNr, const char *DialDesc = "", int MaxDialSec = 10);
    
    // Öffentliche Methode zum Starten der Registrierung (wird in main.cpp genutzt)
    void        Register(); 
    
    void        Cancel(int seqn);
    void        Bye(int cseq);

    
    bool        IsBusy() { return iRingTime != 0; }
    const char* GetSIPServerIP(void);
    char        audioport[7];
    char        lastDtmfDigit = 0; // Speichert die zuletzt gedrückte Taste
};

#endif