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

}


#endif