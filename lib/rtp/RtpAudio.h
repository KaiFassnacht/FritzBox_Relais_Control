#ifndef RtpAudio_h
#define RtpAudio_h

#include <Arduino.h>
#include <WiFiUdp.h>
#include "RTPPacket.h"
#include "g711.h"
#include "settings.h"

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
    void playToneOk()      { playSequence(AudioSequences::OK, sizeof(AudioSequences::OK) / sizeof(ToneStep)); }
    void playToneErr()     { playSequence(AudioSequences::ERR, sizeof(AudioSequences::ERR) / sizeof(ToneStep)); }
    void playToneAlarm()   { playSequence(AudioSequences::ALARM, sizeof(AudioSequences::ALARM) / sizeof(ToneStep)); }
    void playToneTimeout() { playSequence(AudioSequences::TIMEOUT, sizeof(AudioSequences::TIMEOUT) / sizeof(ToneStep)); }
    void playToneStart()   { playSequence(AudioSequences::START, sizeof(AudioSequences::START) / sizeof(ToneStep)); }
    // Die neue universelle Methode
    void playSequence(const ToneStep* steps, size_t count);
    void playTone(float frequency, int durationMs);
};

#endif