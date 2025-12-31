#ifndef RTPAUDIO_H
#define RTPAUDIO_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include "RTPPacket.h"
#include "g711.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "ConfigManager.h"

class RtpAudio {
public:
    RtpAudio();
    void begin(uint16_t localPort);
    void setTarget(const char* ip, uint16_t port);
    void playTone(float frequency, int durationMs);

    // Diese Funktionen rufen nun intern die config-Werte ab
    void playToneOk();
    void playToneErr();
    void playToneAlarm();
    void playToneStart();
    void playToneTimeout();
    void playPinRequest();

private:
    WiFiUDP _udp;
    uint16_t _localPort;
    const char* _remoteIp;
    uint16_t _remotePort;
    uint16_t _seq;
    uint32_t _timestamp;
    uint32_t _ssrc;

    void sendPacket(const uint8_t* payload, size_t size);
    void playSequenceInternal(const ToneConfig* steps, size_t count);
    uint8_t ALaw_Encode(int16_t number); 
};

#endif