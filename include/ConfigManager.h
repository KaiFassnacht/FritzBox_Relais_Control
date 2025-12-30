#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

struct GatewayConfig {
    char sipUser[32] = "611";
    char sipPass[32] = "password";
    char sipRegistrar[64] = "192.168.178.1";
    
    // Korrigierte Standardwerte (KEIN Pin 16/17!)
    // Taste:        0  1   2   3   4   5   6   7   8   9
    int relaisPins[10] = {2, 4, 12, 14, 15, 32, 33, -1, -1, -1};
    char pins[10][8] = {"", "", "", "", "", "", "", "", "", ""};
    
    String whitelist = "**611,**612";
    bool whitelistRequired[10] = {false, false, false, false, false, false, false, false, false, false};
    
    uint32_t relaisDauer = 1000;
    uint32_t timeout = 60000;
};

extern GatewayConfig config;

bool loadConfig();
bool saveConfig();
void validatePins(); // Neu: Schützt die Hardware