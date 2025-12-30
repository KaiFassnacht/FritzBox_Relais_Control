#ifndef SIP_h
#define SIP_h

#include <Arduino.h>
#include <WiFiUdp.h>
#include <MD5Builder.h>
#include "RtpAudio.h"


class Sip {
  private: 
    WiFiUDP     udp;
    char       *pbuf;
    size_t      lbuf;
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

    uint32_t    iRingTime;
    uint32_t    iMaxTime;    // Fehlte im Header
    int         iLastCSeq;

    void        AddSipLine(const char* constFormat , ... );
    bool        AddCopySipLine(const char *p, const char *psearch);   
    bool        ParseParameter(char *dest, int destlen, const char *name, const char *line, char cq = '\"');
    int         GrepInteger(const char *p, const char *psearch);
    void        Ack(const char *pIn);
    void        Ok(const char *pIn);
    void        Invite(const char *pIn = 0);
    void        Register(const char *pIn); 

    int         SendUdp();
    uint32_t    Random();    // Behebt 'Random was not declared'
    void        MakeMd5Digest(char *pOutHex33, char *pIn);

  public: 
    Sip(char *pBuf, size_t lBuf);
    void        Init(const char *SipIp, int SipPort, const char *MyIp, int MyPort, const char *SipUser, const char *SipPassWd);
    void        HandleUdpPacket();
    bool        Dial(const char *DialNr, const char *DialDesc = "", int MaxDialSec = 10);
    void        Register(); 
    void        Cancel(int cseq); // Behebt 'no declaration matches'
    void        Bye(int cseq);

    bool        IsBusy() { return iRingTime != 0; }
    const char* GetSIPServerIP(); // Behebt 'no declaration matches'
    
    char        lastDtmfDigit = 0;
    int         iLastInCSeq = 0;
    RtpAudio    rtp;

    bool isConnected = false;
};

#endif