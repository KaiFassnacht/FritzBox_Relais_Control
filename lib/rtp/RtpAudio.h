#ifndef RtpAudio_h
#define RtpAudio_h

#include <Arduino.h>
#include <WiFiUdp.h>
#include "RTPPacket.h"
#include "g711.h"
#include "settings.h"

// --- Audio Signale ---
#define TONE_OK_FREQ_1     660
#define TONE_OK_FREQ_2     880
#define TONE_OK_DURATION   150

#define TONE_ERR_FREQ_1    300
#define TONE_ERR_FREQ_2    200
#define TONE_ERR_DURATION  200

#define TONE_ALARM_FREQ_1  1000
#define TONE_ALARM_FREQ_2  800
#define TONE_ALARM_FREQ_3  600
#define TONE_ALARM_DURATION 80

#define TONE_TIMEOUT_FREQ_1   440
#define TONE_TIMEOUT_FREQ_2   220
#define TONE_TIMEOUT_DURATION 150

#define TONE_START_FREQ    440  
#define TONE_START_DURATION 100



class RtpAudio {
  private:
    WiFiUDP _udp;
    const char* _remoteIp;
    uint16_t _remotePort;
    uint16_t _localPort;
    
    uint16_t _seq;
    uint32_t _timestamp;
    uint32_t _ssrc;

    void sendPacket(const uint8_t* payload, size_t size);

  public:
    RtpAudio();
    void begin(uint16_t localPort);
    void setTarget(const char* ip, uint16_t port);
    void stop();
    void playToneOk();      // Zwei ansteigende Töne
    void playToneErr();     // Zwei abfallende, tiefe Töne
    void playToneAlarm();   // Sirenen-Effekt (3 Töne abfallend)
    void playToneTimeout(); // Warnung vor dem Auflegen
    void playToneStart();   // Kurzer Bestätigungston beim Abheben

    
    // Spielt einen einfachen Quittungston (blockierend für kurze Dauer)
    void playTone(float frequency, int durationMs);
};

#endif