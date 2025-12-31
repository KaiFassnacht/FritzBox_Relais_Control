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

void RtpAudio::playSequenceInternal(const ToneConfig* steps, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (steps[i].freq > 0) {
            playTone(steps[i].freq, steps[i].duration);
        }
        if (steps[i].pause > 0) {
            delay(steps[i].pause);
        }
    }
}

// --- Wrapper für die einzelnen Töne ---

void RtpAudio::playToneOk() {
    playSequenceInternal(config.toneOk, 2);
}

void RtpAudio::playToneErr() {
    playSequenceInternal(config.toneErr, 2);
}

void RtpAudio::playToneAlarm() {
    playSequenceInternal(config.toneAlarm, 3);
}

void RtpAudio::playToneStart() {
    playSequenceInternal(config.toneStart, 1);
}

void RtpAudio::playToneTimeout() {
    playSequenceInternal(config.toneTimeout, 4);
}

void RtpAudio::playPinRequest() {
    playSequenceInternal(config.tonePinRequest, 4);
}

void RtpAudio::playTone(float frequency, int durationMs) {
    if (_remotePort == 0) return;
    const int samplesPerPacket = 160; 
    uint8_t audioBuf[samplesPerPacket];
    
    // Sicherstellen, dass mindestens 1 Paket gesendet wird
    int numPackets = durationMs / 20;
    if (numPackets < 1) numPackets = 1; 

    uint32_t nextTick = millis();

    for (int p = 0; p < numPackets; p++) {
        for (int i = 0; i < samplesPerPacket; i++) {
            float sample = sin(2.0 * PI * frequency * (p * samplesPerPacket + i) / 8000.0);
            int16_t pcm = (int16_t)(sample * 16384.0); 
            audioBuf[i] = ALaw_Encode(pcm); 
        }
        sendPacket(audioBuf, samplesPerPacket);
        
        nextTick += 20;
        long wait = nextTick - millis();
        if (wait > 0) delay(wait);
    }
}

// A-Law Encoding für G.711 (RTP Standard für Telefonie)
uint8_t RtpAudio::ALaw_Encode(int16_t number) {
    uint16_t mask = 0x800;
    uint8_t sign = 0;
    uint8_t position;
    uint8_t lsb;
    if (number < 0) {
        number = -number;
        sign = 0x80;
    }
    number += 128;
    if (number > 32767) number = 32767;
    for (position = 7; ((number & mask) != mask) && (position > 0); position--) mask >>= 1;
    lsb = (number >> (position + 4)) & 0x0f;
    return (~(sign | (position << 4) | lsb) ^ 0x55);
}