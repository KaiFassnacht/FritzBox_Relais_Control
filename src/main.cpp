#include <Arduino.h>
#include <ETH.h>
#include <LittleFS.h>
#include "sip.h"
#include "ConfigManager.h"
#include "WebHandler.h"

// --- Globale Variablen & Status ---
char myIPAddress[16];
unsigned long letzteAktivitaet = 0;
bool timeoutWarned = false; 
bool startTonePlayed = false; 
String aktuelleEingabe = "";
int geschuetzteTaste = -1; 

char sipBuffer[2048];
Sip sip(sipBuffer, sizeof(sipBuffer));

unsigned long lastRegister = 0;
bool eth_connected = false;

struct RelaisStatus {
    int pin;
    unsigned long startMillis;
    bool aktiv;
};
RelaisStatus relaisListe[10];

WebHandler webHandler;

// --- Hilfsfunktionen ---

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

bool isCallerWhitelisted() {
    // 1. Sicherheits-Check: Wenn die Liste leer ist, ist der Zugriff IMMER verboten
    if (config.whitelist.length() == 0) {
        Serial.println("WHITELIST| Liste ist leer. Zugriff verweigert.");
        return false; 
    }

    String caller = String(sip.caCallerNr);
    caller.trim();

    // 2. Anonymer Anrufer?
    if (caller.length() == 0 || caller.equalsIgnoreCase("anonymous")) {
        Serial.println("WHITELIST| Anonymer Anrufer abgelehnt.");
        return false;
    }

    // 3. Suche in der Liste
    // Wir suchen nach der Nummer. Index -1 bedeutet: Nicht gefunden.
    if (config.whitelist.indexOf(caller) != -1) {
        Serial.printf("WHITELIST| %s gefunden! Zugriff erlaubt.\n", caller.c_str());
        return true;
    }

    Serial.printf("WHITELIST| %s nicht in Liste. Zugriff verweigert.\n", caller.c_str());
    return false;
}

// --- Setup ---

void setup() {
    Serial.begin(115200);
    
    if(!LittleFS.begin(true)) {
        Serial.println("FS| LittleFS Fehler!");
    }

    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while(file){
        Serial.printf("FS| Datei gefunden: %s, Größe: %d\n", file.name(), file.size());
        file = root.openNextFile();
    }
    
    Serial.println("\nSYSTEM| Starte SIP Relais Gateway...");
    // 1. Zuerst Config laden und bereinigen
    loadConfig(); 

    // 2. EXKLUSIV FÜR WT32-ETH01: Ethernet Power einschalten
    // Pin 16 muss HIGH sein, damit der LAN8720 Strom bekommt.
    pinMode(16, OUTPUT);
    digitalWrite(16, HIGH); 
    delay(100); // Kurz warten, bis der Chip stabil läuft
    Serial.println("HARDWARE| PHY Power (GPIO 16) aktiviert.");

    // 3. Erst jetzt die Relais-Pins aus der bereinigten Config setzen
    for (int i = 0; i < 10; i++) {
        int p = config.relaisPins[i];
        
        // WICHTIG: Synchronisiere relaisListe mit der Config!
        relaisListe[i].pin = p; 
        relaisListe[i].aktiv = false;
        relaisListe[i].startMillis = 0;

        if (p != -1) { 
            pinMode(p, OUTPUT);
            digitalWrite(p, LOW);
            Serial.printf("SETUP| Taste %d -> GPIO %d\n", i, p);
        }
    }

    // 4. Ethernet Start
    WiFi.onEvent(WiFiEvent);
    ETH.begin();

    while (!eth_connected) {
        delay(500);
        Serial.print(".");
    }

    strncpy(myIPAddress, ETH.localIP().toString().c_str(), 15);
    
    sip.Init(config.sipRegistrar, config.sipPort, myIPAddress, config.sipPort, config.sipUser, config.sipPass);

    Serial.println("\nSYSTEM| SIP Gateway bereit.");
    sip.Register();
    lastRegister = millis();
    
    webHandler.begin(); // Webserver starten
}

// --- Hauptschleife ---

void loop() {
    // --- SERIELLER RESET & COMMANDS ---
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "factory-reset") {
            Serial.println("SYSTEM| Lösche Konfiguration und starte neu...");
            LittleFS.remove("/config.json");
            delay(1000);
            ESP.restart();
        } 
        else if (cmd == "status") {
            Serial.println("--- AKTUELLER STATUS ---");
            Serial.printf("IP: %s\n", ETH.localIP().toString().c_str());
            Serial.printf("SIP User: %s\n", config.sipUser);
            Serial.print("Relais-Pins: ");
            for(int i=0; i<10; i++) Serial.printf("%d ", config.relaisPins[i]);
            Serial.println("\n-----------------------");
        }
    }
    sip.HandleUdpPacket();

    if (sip.IsBusy()) {
        if (letzteAktivitaet == 0) {
            letzteAktivitaet = millis();
            timeoutWarned = false;
            startTonePlayed = false;
            geschuetzteTaste = -1;
            aktuelleEingabe = "";
            Serial.println("CALL| Eingehender Anruf...");
        }

        if (sip.isConnected && !startTonePlayed) {
            Serial.println("CALL| Handshake komplett.");
            sip.rtp.playToneStart(); 
            startTonePlayed = true;
        }

        if (sip.lastDtmfDigit != 0) {
            letzteAktivitaet = millis(); 
            timeoutWarned = false;       
            
            char taste = sip.lastDtmfDigit;
            sip.lastDtmfDigit = 0; 

            if (sip.isConnected) {
                if (taste == '#') {
                    Serial.println("ACTION| Auflegen via #");
                    sip.Bye(0);
                }
                else if (taste >= '0' && taste <= '9') {
                    int index = taste - '0';
                    
                    // NEU: Wenn die Taste NICHT belegt ist (-1), spiele den Alarm-Ton
                    if (config.relaisPins[index] == -1) {
                        Serial.printf("SIP| Taste %d ist unbelegt -> Spiele Alarm-Ton.\n", index);
                        sip.rtp.playToneAlarm(); // Hier wird der Ton für die unbelegte Taste gesendet
                        return; // Danach abbrechen, da keine weitere Aktion (PIN/Relais) folgen soll
                    }
                    // Whitelist Prüfung aus Config
                    if (config.whitelistRequired[index]) {
                        if (!isCallerWhitelisted()) {
                            // Dieser Block wird jetzt AUCH ausgeführt, wenn die Liste leer ist
                            Serial.printf("SECURITY| Taste %d für Anrufer %s gesperrt!\n", index, sip.caCallerNr);
                            sip.rtp.playToneErr(); // Fehler-Ton
                            return; // Wichtig: Bricht die weitere Verarbeitung ab
                        }
                    }

                    // PIN Eingabe Logik
                    if (geschuetzteTaste != -1) {
                        aktuelleEingabe += taste;
                        String zielPin = String(config.pins[geschuetzteTaste]);
                        
                        if (aktuelleEingabe.length() >= zielPin.length()) {
                            if (aktuelleEingabe == zielPin) {
                                Serial.println("SECURITY| PIN korrekt.");
                                int pin = relaisListe[geschuetzteTaste].pin;
                                if (pin != -1) {
                                    digitalWrite(pin, HIGH);
                                    relaisListe[geschuetzteTaste].aktiv = true;
                                    relaisListe[geschuetzteTaste].startMillis = millis();
                                    sip.rtp.playToneOk();
                                }
                            } else {
                                Serial.println("SECURITY| PIN FALSCH!");
                                sip.rtp.playToneErr();
                            }
                            geschuetzteTaste = -1;
                            aktuelleEingabe = "";
                        }
                    } 
                    else {
                        String benoetigtePin = String(config.pins[index]);
                        if (benoetigtePin.length() > 0) {
                            Serial.println("SECURITY| Warte auf PIN...");
                            geschuetzteTaste = index;
                            aktuelleEingabe = "";
                            sip.rtp.playPinRequest();
                        } else {
                            if (relaisListe[index].pin != -1) {
                                digitalWrite(relaisListe[index].pin, HIGH);
                                relaisListe[index].aktiv = true;
                                relaisListe[index].startMillis = millis();
                                sip.rtp.playToneOk(); 
                            }
                        }
                    }
                }
            }
        }

        // Timeout Management aus Config
        unsigned long dauerAktiv = millis() - letzteAktivitaet;
        if (sip.isConnected && dauerAktiv > (config.timeout - 2000) && !timeoutWarned) {
            sip.rtp.playTimeout(); 
            timeoutWarned = true;
        }
        if (dauerAktiv > config.timeout) {
            sip.Bye(0);
            sip.isConnected = false;
        }

    } else {
        if (letzteAktivitaet != 0) {
            letzteAktivitaet = 0;
            timeoutWarned = false;
            startTonePlayed = false;
            sip.isConnected = false; 
            geschuetzteTaste = -1;
            aktuelleEingabe = "";
        }
    }

    // Impuls-Steuerung aus Config
    for (int i = 0; i < 10; i++) {
        if (relaisListe[i].aktiv) {
            if (millis() - relaisListe[i].startMillis >= config.relaisDauer) {
                digitalWrite(relaisListe[i].pin, LOW);
                relaisListe[i].aktiv = false;
            }
        }
    }

    if (millis() - lastRegister > 240000) {
        sip.Register();
        lastRegister = millis();
    }
}