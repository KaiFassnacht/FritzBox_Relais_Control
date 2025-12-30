#ifndef RtpAudio_h
#define RtpAudio_h

#include <Arduino.h>
#include <WiFiUdp.h>
#include "RTPPacket.h"
#include "g711.h"
#include "settings.h" // In platformio.ini -I include setzen!

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
    void playSequenceInternal(const ToneStep* steps, size_t count);

  public:
    RtpAudio();
    void begin(uint16_t localPort);
    void setTarget(const char* ip, uint16_t port);
    void stop();

    // Template erkennt die Array-Größe automatisch
    template<size_t N>
    void playSequence(const ToneStep (&steps)[N]) {
        playSequenceInternal(steps, N);
    }

    // Bequeme Alias-Methoden
    void playToneOk()      { playSequence(AudioSequences::OK); }
    void playToneErr()     { playSequence(AudioSequences::ERR); }
    void playToneAlarm()   { playSequence(AudioSequences::ALARM); }
    void playToneStart()   { playSequence(AudioSequences::START); }
    void playTimeout()   { playSequence(AudioSequences::TIMEOUT); }

    void playTone(float frequency, int durationMs);
};

#endif