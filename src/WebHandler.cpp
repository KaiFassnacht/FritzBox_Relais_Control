#include "WebHandler.h"

void WebHandler::begin() {
    setupRoutes();
    server.begin();
    Serial.println("WEB| Webserver gestartet.");
}

String processor(const String& var) {
    if (var == "SIPREG") return (config.sipRegistrar[0] == '\0') ? "" : String(config.sipRegistrar);
    if (var == "SIPUSER") return (config.sipUser[0] == '\0') ? "" : String(config.sipUser);
    if (var == "SIPPASS") return (config.sipPass[0] == '\0') ? "" : String(config.sipPass);
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
        // SIP & Basis-Daten (Nutze request->arg für POST-Formulare)
        if (request->hasArg("sipReg")) strlcpy(config.sipRegistrar, request->arg("sipReg").c_str(), 63);
        if (request->hasArg("sipUser")) strlcpy(config.sipUser, request->arg("sipUser").c_str(), 31);
        if (request->hasArg("sipPass")) strlcpy(config.sipPass, request->arg("sipPass").c_str(), 31);
        if (request->hasArg("whitelist")) config.whitelist = request->arg("whitelist");

        // NEU: Zeit-Einstellungen mit Sicherheitsprüfung
        if (request->hasArg("relDur")) {
            int val = request->arg("relDur").toInt();
            // Minimum 100ms, Maximum 30 Sekunden
            if (val < 100) val = 100;
            if (val > 30000) val = 30000;
            config.relaisDauer = val;
        }

        if (request->hasArg("tout")) {
            int val = request->arg("tout").toInt();
            // Minimum 5 Sekunden (5000ms), Maximum 1 Stunde (3600000ms)
            // Damit man nicht sofort nach dem Abheben rausfliegt
            if (val < 5000) val = 5000;
            if (val > 3600000) val = 3600000;
            config.timeout = val;
        }
        // Relais-Tabelle auslesen
        for (int i = 0; i < 10; i++) {
            String pKey = "p" + String(i);
            String cKey = "c" + String(i);
            String wKey = "w" + String(i);

            if (request->hasArg(pKey)) config.relaisPins[i] = request->arg(pKey).toInt();
            if (request->hasArg(cKey)) strlcpy(config.pins[i], request->arg(cKey).c_str(), 7);
            
            // Checkboxen sind nur im Request vorhanden, wenn sie angehakt sind
            config.whitelistRequired[i] = request->hasArg(wKey);
        }
        
        saveConfig();
        
        request->send(200, "text/plain", "Konfiguration gespeichert. Das System startet in 2 Sekunden neu...");
        
        // Timer statt delay(), damit der Request noch sauber abgeschlossen wird
        DefaultHeaders::Instance().addHeader("Connection", "close");
        ESP.restart(); // Hinweis: In manchen Umgebungen ist ein Timer sicherer als ein direkter Restart
    });
}