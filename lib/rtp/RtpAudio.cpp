#include "RtpAudio.h"
#include <math.h>



RtpAudio::RtpAudio() {
    _seq = random(1000, 5000);
    _timestamp = random(10000, 50000);
    _ssrc = random(100000, 999999);
    _remotePort = 0;
}

void RtpAudio::begin(uint16_t localPort) {
    _localPort = localPort;
    _udp.begin(_localPort);
}

void RtpAudio::setTarget(const char* ip, uint16_t port) {
    _remoteIp = ip;
    _remotePort = port;
}

void RtpAudio::sendPacket(const uint8_t* payload, size_t size) {
    if (_remotePort == 0) return;

    uint8_t buffer[size + 12];
    RTPPacket packet(payload, _seq++, _ssrc, _timestamp, 8, size);
    int len = packet.serialize(buffer);
    
    _udp.beginPacket(_remoteIp, _remotePort);
    _udp.write(buffer, len);
    _udp.endPacket();
    
    _timestamp += size;
}

void RtpAudio::playTone(float frequency, int durationMs) {
    if (_remotePort == 0) return;
    const int samplesPerPacket = 160; 
    uint8_t audioBuf[samplesPerPacket];
    int numPackets = durationMs / 20;

    uint32_t nextTick = millis();

    for (int p = 0; p < numPackets; p++) {
        for (int i = 0; i < samplesPerPacket; i++) {
            // Wir nutzen eine lokale Phase für den Ton
            float sample = sin(2.0 * PI * frequency * (p * samplesPerPacket + i) / 8000.0);
            int16_t pcm = (int16_t)(sample * 16384.0); // Etwas lauter als 8000
            audioBuf[i] = ALaw_Encode(pcm); 
        }
        
        sendPacket(audioBuf, samplesPerPacket);
        
        // Präzises 20ms Intervall
        nextTick += 20;
        long wait = nextTick - millis();
        if (wait > 0) delay(wait);
    }
}


void RtpAudio::playToneOk()      { playTone(TONE_OK_FREQ_1, TONE_OK_DURATION); delay(20); playTone(TONE_OK_FREQ_2, TONE_OK_DURATION); }
void RtpAudio::playToneErr()     { playTone(TONE_ERR_FREQ_1, TONE_ERR_DURATION); delay(20); playTone(TONE_ERR_FREQ_2, TONE_ERR_DURATION); }
void RtpAudio::playToneAlarm()   { playTone(TONE_ALARM_FREQ_1, TONE_ALARM_DURATION); playTone(TONE_ALARM_FREQ_2, TONE_ALARM_DURATION); playTone(TONE_ALARM_FREQ_3, TONE_ALARM_DURATION); }
void RtpAudio::playToneTimeout() { playTone(TONE_TIMEOUT_FREQ_1, TONE_TIMEOUT_DURATION); delay(50); playTone(TONE_TIMEOUT_FREQ_2, TONE_TIMEOUT_DURATION); }
void RtpAudio::playToneStart()   { playTone(TONE_START_FREQ, TONE_START_DURATION); }
