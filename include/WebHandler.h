#pragma once
#include <ESPAsyncWebServer.h>
#include "ConfigManager.h"

class WebHandler {
public:
    // Konstruktor: Initialisiert den Server auf Port 80
    WebHandler() : server(80) {}

    // Startet den Webserver (muss in main.cpp aufgerufen werden)
    void begin();

private:
    AsyncWebServer server;

    // Definiert die Routen (/, /save, /hello)
    void setupRoutes();
    // Neue Hilfsmethoden für den Datei-Transfer
    void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
};

// Der Processor muss außerhalb der Klasse deklariert sein, 
// damit er als Callback für den AsyncWebServer fungieren kann.
String processor(const String& var);