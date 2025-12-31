#include "WebHandler.h"
#include <Ticker.h>

Ticker restartTimer;

void WebHandler::begin() {
    setupRoutes();
    server.begin();
    Serial.println("WEB| Webserver gestartet.");
}

String processor(const String& var) {
    if (var == "SIPREG") return (config.sipRegistrar[0] == '\0') ? "" : String(config.sipRegistrar);
    if (var == "SIPUSER") return (config.sipUser[0] == '\0') ? "" : String(config.sipUser);
    if (var == "SIPPASS") return (config.sipPass[0] == '\0') ? "" : String(config.sipPass);
    if (var == "SIPPORT") return String(config.sipPort);
    if (var == "RTPPORT") return String(config.rtpPort);


    if (var == "WHITELIST") return (config.whitelist.length() == 0) ? "" : config.whitelist;
    
    if (var == "RELDUR") return String(config.relaisDauer);
    if (var == "TIMEOUT") return String(config.timeout);
    if (var == "RELAIS_TABLE") {
        String table = "";
        for (int i = 0; i < 10; i++) {
            table += "<tr>";
            table += "<td>" + String(i) + "</td>";
            table += "<td><input type='text' name='n" + String(i) + "' value='" + String(config.relaisNames[i]) + "' maxlength='20' style='width:120px'></td>";
            table += "<td><input type='number' name='p" + String(i) + "' value='" + String(config.relaisPins[i]) + "' style='width:50px'></td>";
            table += "<td><input type='text' name='c" + String(i) + "' value='" + String(config.pins[i]) + "' maxlength='7' style='width:80px'></td>";
            String checked = config.whitelistRequired[i] ? "checked" : "";
            table += "<td><input type='checkbox' name='w" + String(i) + "' " + checked + "></td>";
            table += "</tr>";
        }
        return table;
    }
    
    if (var == "AUDIO_TABLE") {
        String html = "";
        auto genRow = [&](String label, String prefix, ToneConfig* steps, int count) {
            html += "<tr><th colspan='4' style='background:#eee'>" + label + "</th></tr>";
            for (int i = 0; i < count; i++) {
                html += "<tr><td>Schritt " + String(i+1) + "</td>";
                html += "<td><input type='number' name='" + prefix + "f" + String(i) + "' value='" + String(steps[i].freq) + "' placeholder='Hz' style='width:60px'></td>";
                html += "<td><input type='number' name='" + prefix + "d" + String(i) + "' value='" + String(steps[i].duration) + "' placeholder='ms' style='width:60px'></td>";
                html += "<td><input type='number' name='" + prefix + "p" + String(i) + "' value='" + String(steps[i].pause) + "' placeholder='Pause' style='width:60px'></td></tr>";
            }
        };

        genRow("OK-Ton", "ok", config.toneOk, 2);
        genRow("Fehler-Ton", "err", config.toneErr, 2);
        genRow("Alarm (Sirene)", "al", config.toneAlarm, 3);
        genRow("Start-Ton", "st", config.toneStart, 1);
        genRow("Timeout-Warnung", "to", config.toneTimeout, 4);
        genRow("PIN-Anforderung", "pr", config.tonePinRequest, 4);
        
        return html;
    }    
        
    
    return ""; 
}

void WebHandler::setupRoutes() {
    server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Der ESP antwortet!");
    });

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/config.json")) {
            request->send(LittleFS, "/config.json", "application/json");
        } else {
            request->send(404, "text/plain", "Config Datei nicht gefunden.");
        }
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/style.css", "text/css");
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", String(), false, processor);
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(204); 
    });

    server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/audio.html", String(), false, processor);
    });

    server.on("/network", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/network.html", String(), false, processor);
    });
    server.on("/relais", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/relais.html", String(), false, processor);
    });
    server.on("/factory", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/factory.html", String(), false, processor);
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/script.js", "application/javascript");
    });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        // 1. SIP & Ports (mit hasParam)
        if (request->hasParam("sipReg", true)) strlcpy(config.sipRegistrar, request->getParam("sipReg", true)->value().c_str(), 63);
        if (request->hasParam("sipUser", true)) strlcpy(config.sipUser, request->getParam("sipUser", true)->value().c_str(), 31);
        if (request->hasParam("sipPass", true)) strlcpy(config.sipPass, request->getParam("sipPass", true)->value().c_str(), 31);
        
        if (request->hasParam("sipPort", true)) config.sipPort = request->getParam("sipPort", true)->value().toInt();
        if (request->hasParam("rtpPort", true)) config.rtpPort = request->getParam("rtpPort", true)->value().toInt();
        
        if (request->hasParam("whitelist", true)) config.whitelist = request->getParam("whitelist", true)->value();

        // 2. Zeit-Einstellungen
        if (request->hasParam("relDur", true)) {
            int val = request->getParam("relDur", true)->value().toInt();
            config.relaisDauer = (val < 100) ? 100 : (val > 30000 ? 30000 : val);
        }

        if (request->hasParam("tout", true)) {
            int val = request->getParam("tout", true)->value().toInt();
            config.timeout = (val < 5000) ? 5000 : (val > 3600000 ? 3600000 : val);
        }

        // 3. Relais-Tabelle (Schleife mit hasParam)
        for (int i = 0; i < 10; i++) {
            String pKey = "p" + String(i);
            String cKey = "c" + String(i);
            String wKey = "w" + String(i);
            String nKey = "n" + String(i);
            if (request->hasParam(nKey, true)) {
                strlcpy(config.relaisNames[i], request->getParam(nKey, true)->value().c_str(), 21);
            }
            if (request->hasParam(pKey, true)) {
                config.relaisPins[i] = request->getParam(pKey, true)->value().toInt();
            }
            if (request->hasParam(cKey, true)) {
                strlcpy(config.pins[i], request->getParam(cKey, true)->value().c_str(), 7);
            }
            
            // Checkboxen: hasParam liefert true, wenn die Box angehakt wurde
            config.whitelistRequired[i] = request->hasParam(wKey, true);
        }
        
        // 4. Audio-Konfiguration (Lambda Funktion zur Wiederverwendung)
        auto collectAudio = [&](String prefix, ToneConfig* steps, int count) {
            for (int i = 0; i < count; i++) {
                if (request->hasParam(prefix + "f" + String(i), true)) 
                    steps[i].freq = request->getParam(prefix + "f" + String(i), true)->value().toInt();
                if (request->hasParam(prefix + "d" + String(i), true)) 
                    steps[i].duration = request->getParam(prefix + "d" + String(i), true)->value().toInt();
                if (request->hasParam(prefix + "p" + String(i), true)) 
                    steps[i].pause = request->getParam(prefix + "p" + String(i), true)->value().toInt();
            }
        };
        
        collectAudio("ok", config.toneOk, 2);
        collectAudio("err", config.toneErr, 2);
        collectAudio("al", config.toneAlarm, 3);
        collectAudio("st", config.toneStart, 1);
        collectAudio("to", config.toneTimeout, 4);
        collectAudio("pr", config.tonePinRequest, 4);



        saveConfig();
        
        request->send(200, "text/plain", "Konfiguration gespeichert. Das System startet in 2 Sekunden neu...");

        restartTimer.once_ms(2000, []() {
            ESP.restart();
        });
    });
    
    // Separater Save-Handler für Audio (ohne Reboot!)
    server.on("/save-audio", HTTP_POST, [](AsyncWebServerRequest *request) {
        auto collectAudio = [&](String prefix, ToneConfig* steps, int count) {
            for (int i = 0; i < count; i++) {
                if (request->hasParam(prefix + "f" + String(i), true)) 
                    steps[i].freq = request->getParam(prefix + "f" + String(i), true)->value().toInt();
                if (request->hasParam(prefix + "d" + String(i), true)) 
                    steps[i].duration = request->getParam(prefix + "d" + String(i), true)->value().toInt();
                if (request->hasParam(prefix + "p" + String(i), true)) 
                    steps[i].pause = request->getParam(prefix + "p" + String(i), true)->value().toInt();
            }
        };

        collectAudio("ok", config.toneOk, 2);
        collectAudio("err", config.toneErr, 2);
        collectAudio("al", config.toneAlarm, 3);
        collectAudio("st", config.toneStart, 1);
        collectAudio("to", config.toneTimeout, 4);
        collectAudio("pr", config.tonePinRequest, 4);

        saveConfig(); // Speichern auf LittleFS
        request->send(200, "text/plain", "Audio gespeichert");
    });

    // Werkseinstellungen Route
    server.on("/factory-reset", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.println("WEB| Factory Reset angefordert...");
        
        // Datei löschen
        if (LittleFS.exists("/config.json")) {
            LittleFS.remove("/config.json");
        }
        
        request->send(200, "text/plain", "Werkseinstellungen geladen. Neustart...");
        
        // Neustart via Timer, damit die Antwort noch rausgeht
        restartTimer.once_ms(2000, []() {
            ESP.restart();
        });
    });
}