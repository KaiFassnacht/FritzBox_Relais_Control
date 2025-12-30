#include <Arduino.h>
#include <ETH.h>
#include "settings.h"
#include "sip.h"

char myIPAddress[16];
unsigned long letzteAktivitaet = 0;
bool timeoutWarned = false; 
bool startTonePlayed = false; // Neu: Damit der Start-Ton nur einmal kommt
String aktuelleEingabe = "";
int geschuetzteTaste = -1; // -1 bedeutet: wir warten auf die Wahl einer Taste

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
        // TASTEN_MAP ist global (oder Pins::TASTEN_MAP, falls du es verschoben hast)
        relaisListe[i].pin = TASTEN_MAP[i]; 
        relaisListe[i].aktiv = false;
        relaisListe[i].startMillis = 0;

        if (relaisListe[i].pin != Pins::UNUSED) { // Nutze Pins::UNUSED statt -1
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

    // Nutze hier SipConfig::
    sip.Init(SipConfig::SERVER_IP, SipConfig::PORT, myIPAddress, SipConfig::PORT, SipConfig::USER, SipConfig::PASSWORD);
    
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
            geschuetzteTaste = -1; // PIN-Status bei Anrufbeginn zurücksetzen
            aktuelleEingabe = "";
            Serial.println("CALL| Eingehender Anruf...");
        }

        if (sip.isConnected && !startTonePlayed) {
            Serial.println("CALL| Handshake komplett. Sende Bereit-Signal.");
            sip.rtp.playToneStart(); 
            startTonePlayed = true;
        }

        // --- 2. DTMF AUSWERTUNG (Mit PIN-Logik) ---
        if (sip.lastDtmfDigit != 0) {
            letzteAktivitaet = millis(); 
            timeoutWarned = false;       
            
            char taste = sip.lastDtmfDigit;
            sip.lastDtmfDigit = 0; 

            if (sip.isConnected) {
                // FALL # : Sofort Auflegen (Höchste Priorität)
                if (taste == '#') {
                    Serial.println("ACTION| '#' erkannt. Beende Gespräch...");
                    sip.Bye(0);
                    // Reset erfolgt im "else" Teil von IsBusy
                }
                
                // FALL 0-9 : Zifferneingabe
                else if (taste >= '0' && taste <= '9') {
                    int index = taste - '0';

                    // A) Wir befinden uns gerade in einer PIN-Eingabe für eine geschützte Taste
                    if (geschuetzteTaste != -1) {
                        aktuelleEingabe += taste;
                        const char* zielPin = Security::PINS[geschuetzteTaste];
                        
                        // Prüfen, ob die benötigte PIN-Länge erreicht ist
                        if (aktuelleEingabe.length() >= strlen(zielPin)) {
                            if (aktuelleEingabe == String(zielPin)) {
                                Serial.printf("SECURITY| PIN korrekt für Taste %d. Schalte GPIO %d\n", geschuetzteTaste, relaisListe[geschuetzteTaste].pin);
                                if (relaisListe[geschuetzteTaste].pin != Pins::UNUSED) {
                                    digitalWrite(relaisListe[geschuetzteTaste].pin, HIGH);
                                    relaisListe[geschuetzteTaste].aktiv = true;
                                    relaisListe[geschuetzteTaste].startMillis = millis();
                                    sip.rtp.playToneOk();
                                }
                            } else {
                                Serial.println("SECURITY| PIN FALSCH!");
                                sip.rtp.playToneErr();
                            }
                            // Reset PIN-Status nach Versuch (egal ob Erfolg oder Fehler)
                            geschuetzteTaste = -1;
                            aktuelleEingabe = "";
                        }
                    } 
                    // B) Neue Tastenwahl
                    else {
                        const char* benoetigtePin = Security::PINS[index];
                        
                        if (strlen(benoetigtePin) > 0) {
                            // Taste erfordert PIN
                            Serial.printf("SECURITY| Taste %d ist geschützt. Warte auf PIN...\n", index);
                            geschuetzteTaste = index;
                            aktuelleEingabe = "";
                            sip.rtp.playPinRequest(); // Kurzer Hinweiston zur Eingabeaufforderung
                        } else {
                            // Taste ist ungeschützt -> Normal schalten
                            if (relaisListe[index].pin != Pins::UNUSED) {
                                Serial.printf("ACTION| Taste '%c' -> Schalte GPIO %d\n", taste, relaisListe[index].pin);
                                digitalWrite(relaisListe[index].pin, HIGH);
                                relaisListe[index].aktiv = true;
                                relaisListe[index].startMillis = millis();
                                sip.rtp.playToneOk(); 
                            } else {
                                Serial.printf("ALARM| Taste '%c' nicht belegt.\n", taste);
                                sip.rtp.playToneAlarm(); 
                            }
                        }
                    }
                } else {
                    sip.rtp.playToneErr();
                }
            }
        }

        // --- 3. TIMEOUT MANAGEMENT ---
        unsigned long dauerAktiv = millis() - letzteAktivitaet;

        if (sip.isConnected && dauerAktiv > (Timing::INAKTIVITAETS_TIMEOUT - 2000) && !timeoutWarned) {
            Serial.println("CALL| Sende Timeout-Warnung...");
            sip.rtp.playTimeout(); 
            timeoutWarned = true;
        }

        if (dauerAktiv > Timing::INAKTIVITAETS_TIMEOUT) {
            Serial.println("CALL| Timeout erreicht. Sende BYE...");
            sip.Bye(0);
            letzteAktivitaet = 0; 
            sip.isConnected = false;
        }

    } else {
        // Reset, wenn kein Gespräch aktiv ist oder aufgelegt wurde
        if (letzteAktivitaet != 0) {
            Serial.println("CALL| Verbindung getrennt. Reset Status.");
            letzteAktivitaet = 0;
            timeoutWarned = false;
            startTonePlayed = false;
            sip.isConnected = false; 
            geschuetzteTaste = -1; // Wichtig: PIN-Eingabe bei Abbruch löschen
            aktuelleEingabe = "";
        }
        sip.lastDtmfDigit = 0;
    }

    // Zeitsteuerung Relais (Impuls)
    for (int i = 0; i < 10; i++) {
        if (relaisListe[i].aktiv) {
            if (millis() - relaisListe[i].startMillis >= Timing::RELAIS_DAUER) {
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