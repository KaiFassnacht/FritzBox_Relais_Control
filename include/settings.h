#ifndef SETTINGS_H
#define SETTINGS_H

// --- Hardware Pins (Definition der verfügbaren GPIOs) ---
#define PIN_A  2
#define PIN_B  4
#define PIN_C  12
#define PIN_D  14
#define PIN_E  15
#define PIN_F  32
#define PIN_G  33
#define UNUSED -1

// --- Tasten-Zuweisung (Mapping) ---
// Index 0 ist Taste '0', Index 1 ist Taste '1', usw.
// Hier kannst du nun beliebig schieben:
const int TASTEN_MAP[10] = {
    PIN_A,  // Taste '0' -> GPIO 2
    PIN_B,  // Taste '1' -> GPIO 4
    PIN_C,  // Taste '2' -> GPIO 12
    PIN_D,  // Taste '3' -> GPIO 14
    PIN_E,  // Taste '4' -> GPIO 15
    PIN_F,  // Taste '5' -> GPIO 32
    PIN_G,  // Taste '6' -> GPIO 33
    UNUSED, // Taste '7'
    UNUSED, // Taste '8'
    UNUSED  // Taste '9'
};

// --- Zeitsteuerung ---
#define RELAIS_DAUER 3000   // Zeit in ms
#define INAKTIVITAETS_TIMEOUT 10000 // 10 Sekunden in ms


// --- SIP Einstellungen ---
const char* sip_user = "Relaisgateway";
const char* sip_password = "Relaisgateway112";
const char* fritzbox_ip = "192.168.3.1";
const int sip_port = 5060;

#endif