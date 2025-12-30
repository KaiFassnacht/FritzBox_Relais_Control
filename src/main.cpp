#include <Arduino.h>
#include <ETH.h>
#include "settings.h"
#include "sip.h"

char myIPAddress[16];
unsigned long letzteAktivitaet = 0;
bool timeoutWarned = false; 
bool startTonePlayed = false; // Neu: Damit der Start-Ton nur einmal kommt

char sipBuffer[2048];
Sip sip(sipBuffer, sizeof(sipBuffer));

// Timer & Status
unsigned long lastRegister = 0;
bool eth_connected = false;

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START: Serial.println("SYSTEM| ETH Gestartet"); break;
        case ARDUINO_EVENT_ETH_CONNECTED: Serial.println("SYSTEM| ETH Verbunden"); break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.print("SYSTEM| ETH IP erhalten: ");
            Serial.println(ETH.localIP());
            eth_connected = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED: 
            Serial.println("SYSTEM| ETH getrennt!");
            eth_connected = false; 
            break;
        default: break;
    }
}

struct RelaisStatus {
    int pin;
    unsigned long startMillis;
    bool aktiv;
};

RelaisStatus relaisListe[10];

void setup() {
    Serial.begin(115200);
    
    // Initialisierung der Relais-Pins
    for (int i = 0; i < 10; i++) {
        relaisListe[i].pin = TASTEN_MAP[i];
        relaisListe[i].aktiv = false;
        relaisListe[i].startMillis = 0;

        if (relaisListe[i].pin != -1) {
            pinMode(relaisListe[i].pin, OUTPUT);
            digitalWrite(relaisListe[i].pin, LOW);
            Serial.printf("SETUP| Taste %d -> GPIO %d\n", i, relaisListe[i].pin);
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
    
    Serial.println("\nSYSTEM| SIP Gateway bereit.");
    sip.Register();
    lastRegister = millis();
}

void loop() {
    sip.HandleUdpPacket();

    if (sip.IsBusy()) {
        // --- 1. CALL START & ACK CHECK ---
        if (letzteAktivitaet == 0) {
            letzteAktivitaet = millis();
            timeoutWarned = false;
            startTonePlayed = false;
            Serial.println("CALL| Eingehender Anruf...");
        }

        // NEU: Audio erst senden, wenn das ACK da ist (isConnected == true)
        if (sip.isConnected && !startTonePlayed) {
            Serial.println("CALL| Handshake komplett. Sende Bereit-Signal.");
            sip.rtp.playToneStart(); 
            startTonePlayed = true;
        }

        // --- 2. DTMF AUSWERTUNG (Nur wenn verbunden) ---
        if (sip.lastDtmfDigit != 0) {
            letzteAktivitaet = millis(); 
            timeoutWarned = false;       
            
            char taste = sip.lastDtmfDigit;
            sip.lastDtmfDigit = 0; 

            // Töne nur senden, wenn die Leitung steht
            if (sip.isConnected) {
                if (taste >= '0' && taste <= '9') {
                    int index = taste - '0';
                    if (relaisListe[index].pin != -1) {
                        Serial.printf("ACTION| Taste '%c' -> Schalte GPIO %d\n", taste, relaisListe[index].pin);
                        digitalWrite(relaisListe[index].pin, HIGH);
                        relaisListe[index].aktiv = true;
                        relaisListe[index].startMillis = millis();
                        sip.rtp.playToneOk(); 
                    } else {
                        Serial.printf("ALARM| Taste '%c' nicht belegt.\n", taste);
                        sip.rtp.playToneAlarm(); 
                    }
                } else {
                    sip.rtp.playToneErr();
                }
            }
        }

        // --- 3. TIMEOUT MANAGEMENT ---
        unsigned long dauerAktiv = millis() - letzteAktivitaet;

        // Warnung nur senden, wenn verbunden
        if (sip.isConnected && dauerAktiv > (INAKTIVITAETS_TIMEOUT - 2000) && !timeoutWarned) {
            Serial.println("CALL| Sende Timeout-Warnung...");
            sip.rtp.playToneTimeout();
            timeoutWarned = true;
        }

        if (dauerAktiv > INAKTIVITAETS_TIMEOUT) {
            Serial.println("CALL| Timeout erreicht. Sende BYE...");
            sip.Bye(0);
            letzteAktivitaet = 0; 
            sip.isConnected = false; // Sicherheitshalber hier auch resetten
        }

    } else {
        // Reset, wenn kein Gespräch aktiv ist
        if (letzteAktivitaet != 0) {
            Serial.println("CALL| Gespräch beendet.");
            letzteAktivitaet = 0;
            timeoutWarned = false;
            startTonePlayed = false;
            sip.isConnected = false; // WICHTIG: Flag für den nächsten Anruf zurücksetzen
        }
        sip.lastDtmfDigit = 0;
    }

    // Zeitsteuerung Relais (Impuls)
    for (int i = 0; i < 10; i++) {
        if (relaisListe[i].aktiv) {
            if (millis() - relaisListe[i].startMillis >= RELAIS_DAUER) {
                digitalWrite(relaisListe[i].pin, LOW);
                relaisListe[i].aktiv = false;
                Serial.printf("ACTION| Relais %d (GPIO %d) automatisch AUS.\n", i, relaisListe[i].pin);
            }
        }
    }

    // SIP Re-Registration
    if (millis() - lastRegister > 240000) {
        sip.Register();
        lastRegister = millis();
    }
}