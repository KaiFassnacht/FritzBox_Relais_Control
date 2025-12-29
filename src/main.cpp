#include <Arduino.h>
#include <ETH.h>
#include "settings.h"
#include "sip.h"

char myIPAddress[16];

// Puffer für die SIP-Nachrichten (2KB sind ausreichend für die FRITZ!Box)
char sipBuffer[2048];
Sip sip(sipBuffer, sizeof(sipBuffer));

// Timer-Variablen
unsigned long lastRegistration = 0;
const unsigned long registrationInterval = 30 * 60 * 1000; // Alle 30 Minuten (in ms)
bool eth_connected = false;

// Event-Handler für Ethernet
void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            Serial.println("ETH Gestartet");
            ETH.setHostname("WT32-Relaisgateway");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("ETH Verbunden");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.print("ETH IP erhalten: ");
            Serial.println(ETH.localIP());
            eth_connected = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("ETH getrennt!");
            eth_connected = false;
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("ETH Gestoppt");
            eth_connected = false;
            break;
        default:
            break;
    }
}


unsigned long lastRegister = 0;
bool relaisAktiv = false;
unsigned long relaisStartMillis = 0;


void setup() {
    Serial.begin(115200);
    pinMode(RELAIS_PIN, OUTPUT);
    digitalWrite(RELAIS_PIN, LOW);
    // Event-Handler registrieren bevor ETH startet
    WiFi.onEvent(WiFiEvent);
    
    // ETH am WT32-ETH01 starten
    ETH.begin();

    // Warten bis IP vorhanden
    while (!eth_connected) {
        delay(500);
        Serial.print(".");
    }

    strncpy(myIPAddress, ETH.localIP().toString().c_str(), 15);
    myIPAddress[15] = '\0'; // Sicherstellen, dass der Text endet

    // 3. Jetzt die Init-Funktion mit der festen Variable aufrufen
    sip.Init(fritzbox_ip, 
             sip_port, 
             myIPAddress,  // <--- Hier nutzen wir jetzt den festen Speicher
             sip_port, 
             sip_user, 
             sip_password);

    Serial.println("\nSIP Initialisiert. Starte Erst-Registrierung...");
    sip.Register(); // Die erste Anmeldung an der Box
    lastRegistration = millis();
}

void loop() {
    // 1. SIP-Pakete verarbeiten (Extrem wichtig!)
    sip.HandleUdpPacket();

// PRÜFUNG: Wurde gerade ein Anruf angenommen?
    if (sip.IsBusy() && !relaisAktiv) {
        Serial.println("SYSTEM| Anruf erkannt - Relais AN");
        digitalWrite(RELAIS_PIN, HIGH);
        relaisAktiv = true;
        relaisStartMillis = millis();
    }

    // PRÜFUNG: Ist die Zeit aus den Settings abgelaufen?
    if (relaisAktiv) {
        if (millis() - relaisStartMillis >= RELAIS_DAUER) {
            Serial.println("SYSTEM| Zeit abgelaufen. Relais AUS und lege aktiv auf.");
            digitalWrite(RELAIS_PIN, LOW);
            relaisAktiv = false;
            
            // Wir rufen die vorhandene Bye-Funktion auf. 
            // Wir nehmen eine feste höhere CSeq (z.B. 10), damit die Fritzbox es akzeptiert.
            sip.Bye(10); 
            
            // Wichtig: Den Status in der SIP Klasse auch zurücksetzen, 
            // damit IsBusy() wieder false wird.
            // Falls iRingTime private ist, brauchen wir eine Methode dafür oder 
            // wir setzen sie in der sip.cpp innerhalb von Bye auf 0.
        }
    }
    // Re-Registrierung alle 4 Minuten (300s / 1.2)
    if (millis() - lastRegister > 240000) {
        sip.Register();
        lastRegister = millis();
    }




}