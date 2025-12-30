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
};

// Der Processor muss außerhalb der Klasse deklariert sein, 
// damit er als Callback für den AsyncWebServer fungieren kann.
String processor(const String& var);