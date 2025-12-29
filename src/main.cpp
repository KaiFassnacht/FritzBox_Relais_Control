#include <Arduino.h>
#include <ETH.h>
#include "settings.h"
#include "sip.h"

char myIPAddress[16];
unsigned long letzteAktivitaet = 0;

char sipBuffer[2048];
Sip sip(sipBuffer, sizeof(sipBuffer));

// Timer & Status
unsigned long lastRegister = 0;
bool eth_connected = false;


void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START: Serial.println("ETH Gestartet"); break;
        case ARDUINO_EVENT_ETH_CONNECTED: Serial.println("ETH Verbunden"); break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.print("ETH IP erhalten: ");
            Serial.println(ETH.localIP());
            eth_connected = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED: eth_connected = false; break;
        default: break;
    }
}

struct RelaisStatus {
    int pin;
    unsigned long startMillis;
    bool aktiv;
};

// Das Array wird nun automatisch aus der settings.h befüllt
RelaisStatus relaisListe[10];

void setup() {
    Serial.begin(115200);
    
    // Initialisierung basierend auf dem Mapping in settings.h
    for (int i = 0; i < 10; i++) {
        relaisListe[i].pin = TASTEN_MAP[i]; // Zuweisung aus settings.h
        relaisListe[i].aktiv = false;
        relaisListe[i].startMillis = 0;

        if (relaisListe[i].pin != -1) {
            pinMode(relaisListe[i].pin, OUTPUT);
            digitalWrite(relaisListe[i].pin, LOW);
            Serial.printf("SETUP| Taste %d zugewiesen an GPIO %d\n", i, relaisListe[i].pin);
        }
    }

    WiFi.onEvent(WiFiEvent);
    ETH.begin();

    while (!eth_connected) {
        delay(500);
        Serial.print(".");
    }

    strncpy(myIPAddress, ETH.localIP().toString().c_str(), 15);
    myIPAddress[15] = '\0';

    sip.Init(fritzbox_ip, sip_port, myIPAddress, sip_port, sip_user, sip_password);
    
    Serial.println("\nSIP Gateway bereit. 7 Relais aktiv (Tasten 0-6).");
    sip.Register();
    lastRegister = millis();
}

void loop() {
    sip.HandleUdpPacket();

    if (sip.IsBusy()) {
        // Initialer Zeitstempel, wenn der Anruf gerade erst startete
        if (letzteAktivitaet == 0) {
            letzteAktivitaet = millis();
        }

        if (sip.lastDtmfDigit != 0) {
            // BEI TASTENDRUCK: Timer zurücksetzen
            letzteAktivitaet = millis(); 
            
            char taste = sip.lastDtmfDigit;
            sip.lastDtmfDigit = 0; 
            
            if (taste >= '0' && taste <= '9') {
                int index = taste - '0';
                if (relaisListe[index].pin != -1) {
                    Serial.printf("SYSTEM| Taste '%c' -> Schalte GPIO %d\n", taste, relaisListe[index].pin);
                    digitalWrite(relaisListe[index].pin, HIGH);
                    relaisListe[index].aktiv = true;
                    relaisListe[index].startMillis = millis();
                }
            }
        }

        // PRÜFUNG: 10 Sekunden nichts passiert?
        if (millis() - letzteAktivitaet > INAKTIVITAETS_TIMEOUT) {
            Serial.println("SYSTEM| Inaktivitäts-Timeout! Lege auf...");
            sip.Bye(10); // Aktiv auflegen
            letzteAktivitaet = 0; // Reset für den nächsten Anruf
        }

    } else {
        // Wenn nicht busy, Timer auf 0 halten
        letzteAktivitaet = 0;
        sip.lastDtmfDigit = 0;
    }

    // --- Zeitsteuerung Relais-Ausschalten (bleibt wie es war) ---
    for (int i = 0; i < 10; i++) {
        if (relaisListe[i].aktiv) {
            if (millis() - relaisListe[i].startMillis >= RELAIS_DAUER) {
                digitalWrite(relaisListe[i].pin, LOW);
                relaisListe[i].aktiv = false;
                Serial.printf("SYSTEM| Relais an Taste %d (GPIO %d) aus.\n", i, relaisListe[i].pin);
            }
        }
    }

    // --- SIP Registrierung ---
    if (millis() - lastRegister > 240000) {
        sip.Register();
        lastRegister = millis();
    }
}