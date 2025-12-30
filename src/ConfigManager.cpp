#include "ConfigManager.h"

GatewayConfig config;
const char* configFile = "/config.json";

// Verhindert, dass System-Pins des WT32-ETH01 gekapert werden
void validatePins() {
    bool changed = false;
    for (int i = 0; i < 10; i++) {
        int p = config.relaisPins[i];
        // 16 = ETH Power, 17 = PHY Clock (intern), 23/18 = MDC/MDIO
        if (p == 16 || p == 17 || p == 23 || p == 18 || p == 0) {
            Serial.printf("SAFE| Pin %d auf Taste %d ist reserviert! Deaktiviert.\n", p, i);
            config.relaisPins[i] = -1;
            changed = true;
        }
    }
    // Falls wir beim Laden korrigieren mussten, direkt sauber speichern
    if (changed) saveConfig();
}

bool loadConfig() {
    if (!LittleFS.exists(configFile)) {
        Serial.println("CFG| Keine Config gefunden, Standard wird genutzt.");
        validatePins(); // Sicherstellen, dass Defaults ok sind
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

    strlcpy(config.sipUser, doc["sipUser"] | "611", sizeof(config.sipUser));
    strlcpy(config.sipPass, doc["sipPass"] | "", sizeof(config.sipPass));
    strlcpy(config.sipRegistrar, doc["sipRegistrar"] | "", sizeof(config.sipRegistrar));
    config.whitelist = doc["whitelist"] | "";
    config.relaisDauer = doc["relaisDauer"] | 1000;
    config.timeout = doc["timeout"] | 60000;

    for (int i = 0; i < 10; i++) {
        config.relaisPins[i] = doc["relaisPins"][i] | -1;
        strlcpy(config.pins[i], doc["pins"][i] | "", sizeof(config.pins[i]));
        config.whitelistRequired[i] = doc["whitelistReq"][i] | false;
    }

    validatePins(); // Hardware-Check nach dem Laden
    Serial.println("CFG| Einstellungen geladen.");
    return true;
}

bool saveConfig() {
    // Bevor wir Schrott speichern, Pins prüfen
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