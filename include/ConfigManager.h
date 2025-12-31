#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

struct ToneConfig {
    uint16_t freq;
    uint16_t duration;
    uint16_t pause;
};

struct GatewayConfig {
    char sipUser[32] = "611";
    char sipPass[32] = "password";
    char sipRegistrar[64] = "192.168.178.1";
    uint16_t sipPort = 5060;     // Neu
    uint16_t rtpPort = 10002;     // Neu
    
    // Korrigierte Standardwerte (KEIN Pin 16/17!)
    // Taste:        0  1   2   3   4   5   6   7   8   9
    int relaisPins[10] = {2, 4, 12, 14, 15, 32, 33, -1, -1, -1};
    char pins[10][8] = {"", "", "", "", "", "", "", "", "", ""};

    char relaisNames[10][21]; // 10 Namen mit max. 20 Zeichen + Nullterminator

    String whitelist = "**611,**612";
    bool whitelistRequired[10] = {false, false, false, false, false, false, false, false, false, false};
    
    uint32_t relaisDauer = 1000;
    uint32_t timeout = 60000;

    // OK-Ton: 2 Schritte
    ToneConfig toneOk[2] = {
        {660, 150, 20}, 
        {880, 150, 0}
    };

    // Error-Ton: 2 Schritte
    ToneConfig toneErr[2] = {
        {300, 200, 20}, 
        {200, 200, 0}
    };

    // Alarm: 3 Schritte (Sirene)
    ToneConfig toneAlarm[3] = {
        {1000, 80, 0}, 
        {800, 80, 0}, 
        {600, 80, 0}
    };

    // Start-Ton: 1 Schritt
    ToneConfig toneStart[1] = {
        {440, 200, 0}
    };

    // Timeout: 4 Schritte (2 Schläge, jeweils wiederholt)
    ToneConfig toneTimeout[4] = {
        {150, 60, 100}, 
        {150, 60, 0},
        {150, 60, 100}, 
        {150, 60, 0}
    };

    // Pin-Request: 4 Schritte (Zweistufiger Hinweiston, wiederholt)
    ToneConfig tonePinRequest[4] = {
        {880, 80, 40}, 
        {1046, 120, 0},
        {880, 80, 40}, 
        {1046, 120, 0}
    };
};

extern GatewayConfig config;

bool loadConfig();
bool saveConfig();
void validatePins(); // Neu: Schützt die Hardware