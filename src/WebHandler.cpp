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
            table += "<td><input type='number' name='p" + String(i) + "' value='" + String(config.relaisPins[i]) + "' style='width:50px'></td>";
            table += "<td><input type='text' name='c" + String(i) + "' value='" + String(config.pins[i]) + "' maxlength='7' style='width:80px'></td>";
            String checked = config.whitelistRequired[i] ? "checked" : "";
            table += "<td><input type='checkbox' name='w" + String(i) + "' " + checked + "></td>";
            table += "</tr>";
        }
        return table;
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

            if (request->hasParam(pKey, true)) {
                config.relaisPins[i] = request->getParam(pKey, true)->value().toInt();
            }
            if (request->hasParam(cKey, true)) {
                strlcpy(config.pins[i], request->getParam(cKey, true)->value().c_str(), 7);
            }
            
            // Checkboxen: hasParam liefert true, wenn die Box angehakt wurde
            config.whitelistRequired[i] = request->hasParam(wKey, true);
        }
        
        saveConfig();
        
        request->send(200, "text/plain", "Konfiguration gespeichert. Das System startet in 2 Sekunden neu...");

        restartTimer.once_ms(2000, []() {
            ESP.restart();
        });
    });    
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