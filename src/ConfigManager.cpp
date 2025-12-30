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

// Hilfsfunktion: Setzt alle Werte auf sichere Standards
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

    // Deine GPIO Belegung für Tasten 0 bis 6
    int defaultPins[] = {2, 4, 12, 14, 15, 32, 33};

    for (int i = 0; i < 10; i++) {
        if (i < 7) {
            config.relaisPins[i] = defaultPins[i];
        } else {
            config.relaisPins[i] = -1;
        }
        memset(config.pins[i], 0, sizeof(config.pins[i]));
        config.whitelistRequired[i] = false;
    }
}

bool loadConfig() {
    // 1. IMMER zuerst Defaults setzen
    setDefaults();

    // 2. Prüfen ob Datei existiert
    if (!LittleFS.exists(configFile)) {
        Serial.println("CFG| Keine Datei gefunden. Nutze Defaults und erstelle Datei...");
        validatePins(); 
        saveConfig(); // Erstellt die Datei mit den soeben gesetzten Defaults
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

    // 3. Nur überschreiben, was in der JSON steht (Rest bleibt Default)
    if (doc["sipUser"].is<const char*>()) strlcpy(config.sipUser, doc["sipUser"], sizeof(config.sipUser));
    if (doc["sipPass"].is<const char*>()) strlcpy(config.sipPass, doc["sipPass"], sizeof(config.sipPass));
    if (doc["sipRegistrar"].is<const char*>()) strlcpy(config.sipRegistrar, doc["sipRegistrar"], sizeof(config.sipRegistrar));    
    config.sipPort = doc["sipPort"] | config.sipPort;
    config.rtpPort = doc["rtpPort"] | config.rtpPort;
    config.whitelist = doc["whitelist"] | config.whitelist;
    config.relaisDauer = doc["relaisDauer"] | config.relaisDauer;
    config.timeout = doc["timeout"] | config.timeout;

    for (int i = 0; i < 10; i++) {
        config.relaisPins[i] = doc["relaisPins"][i] | -1;
        if (doc["pins"][i]) strlcpy(config.pins[i], doc["pins"][i], sizeof(config.pins[i]));
        config.whitelistRequired[i] = doc["whitelistReq"][i] | false;
    }

    validatePins(); 
    Serial.println("CFG| Einstellungen geladen.");
    return true;
}

bool saveConfig() {
    // Vor dem Speichern Pins validieren
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
    doc["sipPort"] = config.sipPort;   // NEU
    doc["rtpPort"] = config.rtpPort;   // NEU
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

    serializeJson(doc, file);
    file.close();
    Serial.println("CFG| Erfolgreich gespeichert.");
    return true;
}