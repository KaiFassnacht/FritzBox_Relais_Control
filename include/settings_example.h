#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

// --- Hardware Pins ---
// constexpr ist typsicher und wird zur Kompilierzeit aufgelöst
namespace Pins {
    constexpr int8_t A = 2;
    constexpr int8_t B = 4;
    constexpr int8_t C = 12;
    constexpr int8_t D = 14;
    constexpr int8_t E = 15;
    constexpr int8_t F = 32;
    constexpr int8_t G = 33;
    constexpr int8_t UNUSED = -1;
}

// --- Tasten-Zuweisung ---
// Mit constexpr direkt im Header definierbar
constexpr int TASTEN_MAP[10] = {
    Pins::A, Pins::B, Pins::C, Pins::D, Pins::E, 
    Pins::F, Pins::G, Pins::UNUSED, Pins::UNUSED, Pins::UNUSED
};

// --- Zeitsteuerung ---
namespace Timing {
    constexpr uint32_t RELAIS_DAUER = 3000;
    constexpr uint32_t INAKTIVITAETS_TIMEOUT = 5000;
}

// --- SIP Einstellungen ---
namespace SipConfig {
    constexpr const char* USER = "<SIPuser>";
    constexpr const char* PASSWORD = "<SIPpasswort>";
    constexpr const char* SERVER_IP = "<FritzboxIP>";
    constexpr uint16_t PORT = 5060;
    constexpr uint16_t RTP_PORT = 10002;
}


// --- Audio Signale Konfiguration ---
struct ToneStep {
    uint16_t freq;
    uint16_t duration;
    uint16_t pause; // Pause NACH dem Ton
};

namespace AudioSequences {
    // OK-Ton: 2 ansteigende Töne
    const ToneStep OK[] = {
        {660, 150, 20},
        {880, 150, 0}
    };

    // Error-Ton: 2 abfallende Töne
    const ToneStep ERR[] = {
        {300, 200, 20},
        {200, 200, 0}
    };

    // Alarm: 3 abfallende Töne (Sirene)
    const ToneStep ALARM[] = {
        {1000, 80, 0},
        {800, 80, 0},
        {600, 80, 0}
    };

    // Start-Ton: Einzelner Piep
    const ToneStep START[] = {
        {440, 100, 0}
    };

    // Warnung vor dem Auflegen: 2 Töne mit deutlicher Pause
    const ToneStep TIMEOUT[] = {
        {150, 60, 100}, // Erster Schlag (tief), 100ms Pause
        {150, 60, 0},    // Zweiter Schlag    };
        {150, 60, 100}, // Erster Schlag (tief), 100ms Pause
        {150, 60, 0}    // Zweiter Schlag    };
    };
    
    // Ein eleganter, zweistufiger Hinweiston (Frequenzen 880Hz und 1046Hz)
    constexpr ToneStep PIN_REQUEST[] = {
        {880, 80, 40},  // Ton A: 880Hz, 80ms lang, 40ms Pause
        {1046, 120, 0},  // Ton B: 1046Hz (C6), 120ms lang
        {880, 80, 40},  // Ton A: 880Hz, 80ms lang, 40ms Pause
        {1046, 120, 0}  // Ton B: 1046Hz (C6), 120ms lang
    };

}

namespace Security {
    // Wenn der String leer ist "", ist die Taste ungeschützt.
    // Ansonsten muss genau dieser PIN eingegeben werden.
    constexpr const char* PINS[10] = {
        "",     // Taste 0:
        "",     // Taste 1:
        "1111", // Taste 2:
        "",     // Taste 3:
        "",     // Taste 4:
        "",     // Taste 5:
        "",     // Taste 6:
        "",     // Taste 7:
        "",     // Taste 8:
        ""      // Taste 9:
    };
    // Liste der erlaubten Rufnummern (Whitelist)
    constexpr const char* WHITELIST[] = {
        "01711234567", 
        "**611" // Interne Nummer der FritzBox
    };
    constexpr int WHITELIST_SIZE = sizeof(WHITELIST) / sizeof(WHITELIST[0]);

    // Welche Tasten prüfen die Whitelist? (true = nur für Whitelist-Kontakte)
    constexpr bool WHITELIST_REQUIRED[10] = {
        false,   // Taste 0:
        true,   // Taste 1:
        true,   // Taste 2:
        true,   // Taste 3:
        true,   // Taste 4:
        false,  // Taste 5:
        false,  // Taste 6:
        false,  // Taste 7:
        false,  // Taste 8:
        false   // Taste 9:
    };



}

#endif