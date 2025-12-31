#include "ConfigManager.h"

GatewayConfig config;
const char* configFile = "/config.json";

void validatePins() {
    bool changed = false;
    for (int i = 0; i < 10; i++) {
        int p = config.relaisPins[i];
        if (p == 16 || p == 17 || p == 23 || p == 18 || p == 0) {
            Serial.printf("SAFE| Pin %d auf Taste %d ist reserviert! Deaktiviert.\n", p, i);
            config.relaisPins[i] = -1;
            changed = true;
        }
    }
    if (changed) saveConfig();
}

// Hilfsfunktion: Setzt alle Werte auf sichere Standards inklusive Audio
void setDefaults() {
    Serial.println("CFG| Setze Standardwerte...");
    strlcpy(config.sipUser, "", sizeof(config.sipUser));
    strlcpy(config.sipPass, "", sizeof(config.sipPass));
    strlcpy(config.sipRegistrar, "192.168.178.1", sizeof(config.sipRegistrar));
    config.sipPort = 5060;
    config.rtpPort = 10002;
    config.relaisDauer = 1000;
    config.timeout = 60000;
    config.whitelist = "";

    int defaultPins[] = {2, 4, 12, 14, 15, 32, 33};
    for (int i = 0; i < 10; i++) {
        config.relaisPins[i] = (i < 7) ? defaultPins[i] : -1;
        memset(config.pins[i], 0, sizeof(config.pins[i]));
        config.whitelistRequired[i] = false;
        snprintf(config.relaisNames[i], 21, "Relais %d", i);
    }

    // Audio Defaults (wie in deiner settings.h definiert)
    config.toneOk[0] = {660, 150, 20}; config.toneOk[1] = {880, 150, 0};
    config.toneErr[0] = {300, 200, 20}; config.toneErr[1] = {200, 200, 0};
    config.toneAlarm[0] = {1000, 80, 0}; config.toneAlarm[1] = {800, 80, 0}; config.toneAlarm[2] = {600, 80, 0};
    config.toneStart[0] = {440, 200, 0};
    config.toneTimeout[0] = {150, 60, 100}; config.toneTimeout[1] = {150, 60, 0};
    config.toneTimeout[2] = {150, 60, 100}; config.toneTimeout[3] = {150, 60, 0};
    config.tonePinRequest[0] = {880, 80, 40}; config.tonePinRequest[1] = {1046, 120, 0};
    config.tonePinRequest[2] = {880, 80, 40}; config.tonePinRequest[3] = {1046, 120, 0};
    
}

bool loadConfig() {
    setDefaults();

    if (!LittleFS.exists(configFile)) {
        Serial.println("CFG| Keine Datei gefunden. Erstelle Datei...");
        validatePins(); 
        saveConfig();
        return true;
    }

    File file = LittleFS.open(configFile, "r");
    if (!file) return false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("CFG| JSON Fehler! Nutze Defaults.");
        return false;
    }

    // Basis & SIP
    if (doc["sipUser"].is<const char*>()) strlcpy(config.sipUser, doc["sipUser"], sizeof(config.sipUser));
    if (doc["sipPass"].is<const char*>()) strlcpy(config.sipPass, doc["sipPass"], sizeof(config.sipPass));
    if (doc["sipRegistrar"].is<const char*>()) strlcpy(config.sipRegistrar, doc["sipRegistrar"], sizeof(config.sipRegistrar));    
    config.sipPort = doc["sipPort"] | config.sipPort;
    config.rtpPort = doc["rtpPort"] | config.rtpPort;
    config.whitelist = doc["whitelist"] | config.whitelist;
    config.relaisDauer = doc["relaisDauer"] | config.relaisDauer;
    config.timeout = doc["timeout"] | config.timeout;

    // Relais
    for (int i = 0; i < 10; i++) {
        config.relaisPins[i] = doc["relaisPins"][i] | config.relaisPins[i];
        if (doc["pins"][i]) strlcpy(config.pins[i], doc["pins"][i], sizeof(config.pins[i]));
        config.whitelistRequired[i] = doc["whitelistReq"][i] | config.whitelistRequired[i];
        if (doc["relaisNames"][i].is<const char*>()) {
            strlcpy(config.relaisNames[i], doc["relaisNames"][i], 21);
        }
    }

    // Audio-Sequenzen laden
    auto loadAudio = [&](const char* key, ToneConfig* steps, int count) {
        if (doc[key].is<JsonArray>()) {
            for (int i = 0; i < count; i++) {
                if (doc[key][i].is<JsonArray>()) {
                    steps[i].freq = doc[key][i][0] | steps[i].freq;
                    steps[i].duration = doc[key][i][1] | steps[i].duration;
                    steps[i].pause = doc[key][i][2] | steps[i].pause;
                }
            }
        }
    };

    loadAudio("tOk", config.toneOk, 2);
    loadAudio("tErr", config.toneErr, 2);
    loadAudio("tAl", config.toneAlarm, 3);
    loadAudio("tSt", config.toneStart, 1);
    loadAudio("tTo", config.toneTimeout, 4);
    loadAudio("tPr", config.tonePinRequest, 4);

    validatePins(); 
    Serial.println("CFG| Einstellungen geladen.");
    return true;
}

bool saveConfig() {
    // Hardware-Schutz
    for (int i = 0; i < 10; i++) {
        int p = config.relaisPins[i];
        if (p == 16 || p == 17 || p == 23 || p == 18 || p == 0) config.relaisPins[i] = -1;
    }

    File file = LittleFS.open(configFile, "w");
    if (!file) return false;

    JsonDocument doc;
    doc["sipUser"] = config.sipUser;
    doc["sipPass"] = config.sipPass;
    doc["sipRegistrar"] = config.sipRegistrar;
    doc["sipPort"] = config.sipPort;
    doc["rtpPort"] = config.rtpPort;
    doc["whitelist"] = config.whitelist;
    doc["relaisDauer"] = config.relaisDauer;
    doc["timeout"] = config.timeout;

    JsonArray rPins = doc["relaisPins"].to<JsonArray>();
    JsonArray pCodes = doc["pins"].to<JsonArray>();
    JsonArray wReq = doc["whitelistReq"].to<JsonArray>();
    for (int i = 0; i < 10; i++) {
        rPins.add(config.relaisPins[i]);
        pCodes.add(config.pins[i]);
        wReq.add(config.whitelistRequired[i]);
    }
    JsonArray names = doc["relaisNames"].to<JsonArray>();
    for (int i = 0; i < 10; i++) {
        names.add(config.relaisNames[i]);
    }

    // Audio-Sequenzen speichern
    auto saveAudio = [&](const char* key, ToneConfig* steps, int count) {
        JsonArray toneArr = doc[key].to<JsonArray>();
        for (int i = 0; i < count; i++) {
            JsonArray step = toneArr.add<JsonArray>();
            step.add(steps[i].freq);
            step.add(steps[i].duration);
            step.add(steps[i].pause);
        }
    };

    saveAudio("tOk", config.toneOk, 2);
    saveAudio("tErr", config.toneErr, 2);
    saveAudio("tAl", config.toneAlarm, 3);
    saveAudio("tSt", config.toneStart, 1);
    saveAudio("tTo", config.toneTimeout, 4);
    saveAudio("tPr", config.tonePinRequest, 4);

    serializeJson(doc, file);
    file.close();
    Serial.println("CFG| Erfolgreich gespeichert.");
    return true;
}