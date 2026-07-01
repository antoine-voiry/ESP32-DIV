// include/proximity_wand.h
// Hardware interface layer for the Proximity Wand module.
// SPI bus deassert macro, OUI flash table, UI palette, module interface.
#ifndef PROXIMITY_WAND_H
#define PROXIMITY_WAND_H

#include <Arduino.h>
#include "proximity_logic.h"

// ── SPI Bus Safety Macro ──────────────────────────────────────────────────────
// Forces all four CS/CSN lines HIGH and waits 10 µs for line settling.
// Must be called before any peripheral asserts its own CS LOW.
//
// Pin assignments per docs/hardware/PINOUT_V1.md:
//   GPIO 15 = TFT CS  (HSPI)
//   GPIO 27 = CC1101 CSN / NRF24 radio2 CSN  (VSPI)
//   GPIO 17 = NRF24 radio1 CSN               (VSPI)
//   GPIO  5 = SD CS   / NRF24 radio3 CSN     (VSPI)
#define DEASSERT_SPI_BUS() do {                      \
    pinMode(15, OUTPUT); digitalWrite(15, HIGH);     \
    pinMode(27, OUTPUT); digitalWrite(27, HIGH);     \
    pinMode(17, OUTPUT); digitalWrite(17, HIGH);     \
    pinMode(5,  OUTPUT); digitalWrite(5,  HIGH);     \
    delayMicroseconds(10);                           \
} while (0)

// ── OUI Lookup Table — 28 entries + sentinel, stored in flash ────────────────
// On ESP32 'const' already maps to the .rodata flash segment;
// PROGMEM is retained for explicit intent and AVR toolchain compatibility.
static const OuiEntry OUI_TABLE[] PROGMEM = {
    //  OUI bytes               Vendor name     Threat
    {{0xAC, 0xDE, 0x48}, "Apple",       false},
    {{0x00, 0x17, 0xF2}, "Apple",       false},
    {{0x04, 0x15, 0xEA}, "Apple",       false},
    {{0xFC, 0x3D, 0x93}, "AirTag",      true },
    {{0x00, 0x0D, 0x93}, "AirTag",      true },
    {{0x7C, 0x04, 0xD0}, "AirTag",      true },  // reported Apple AirTag OUI
    {{0x00, 0x1A, 0x11}, "Google",      false},
    {{0x54, 0x60, 0x09}, "Google",      false},
    {{0xE0, 0x28, 0x6D}, "Cisco",       false},
    {{0xA0, 0xEC, 0xF9}, "Cisco",       false},
    {{0x60, 0x60, 0x1F}, "DJI",         true },  // drone
    {{0x34, 0xD2, 0x62}, "DJI",         true },
    {{0xBA, 0x29, 0x40}, "DJI",         true },
    {{0x00, 0x15, 0x6D}, "Ubiquiti",    false},
    {{0x24, 0xA4, 0x3C}, "Ubiquiti",    false},
    {{0xE4, 0x95, 0x6E}, "Samsung",     false},
    {{0x00, 0x16, 0xF0}, "Samsung",     false},
    {{0xB8, 0x27, 0xEB}, "RPi Fdn",     false},
    {{0xDC, 0xA6, 0x32}, "RPi Fdn",     false},
    {{0x00, 0x0C, 0xE7}, "Tile",        true },  // tracker
    {{0x7C, 0x9E, 0xBD}, "Tile",        true },
    {{0x58, 0x8E, 0x81}, "Chipolo",     true },  // tracker
    {{0x00, 0x26, 0xB9}, "Nokia/HMD",   false},
    {{0x00, 0x1B, 0x21}, "Intel",       false},
    {{0x28, 0xD2, 0x44}, "Espressif",   false},
    {{0xA0, 0x20, 0xA6}, "Espressif",   false},
    {{0xFC, 0x45, 0x4D}, "Amazon",      true },  // Echo / tracking
    {{0x68, 0x37, 0xE9}, "Amazon",      true },
    {{0x00, 0x00, 0x00}, "",            false},  // sentinel — must be last
};
static const uint8_t OUI_TABLE_LEN =
    sizeof(OUI_TABLE) / sizeof(OUI_TABLE[0]) - 1;  // excludes sentinel

// ── UI Palette — RGB565 high-contrast industrial ─────────────────────────────
static const uint16_t PW_BG       = 0x0000;  // midnight black
static const uint16_t PW_TEXT     = 0xFFFF;  // stark white
static const uint16_t PW_AMBER    = 0xFD20;  // vivid amber (highlight / label)
static const uint16_t PW_DIM      = 0x4208;  // dark grey — ZONE_NONE bar
static const uint16_t PW_GREEN    = 0x07E0;  // ZONE_LOW
static const uint16_t PW_YELLOW   = 0xFFE0;  // ZONE_MODERATE
static const uint16_t PW_RED      = 0xF800;  // ZONE_CRITICAL
// Guard against PROX_COLOR_* in proximity_logic.h drifting from PW_* palette.
static_assert(PW_DIM    == PROX_COLOR_NONE,     "PROX_COLOR_NONE / PW_DIM mismatch");
static_assert(PW_GREEN  == PROX_COLOR_LOW,      "PROX_COLOR_LOW / PW_GREEN mismatch");
static_assert(PW_YELLOW == PROX_COLOR_MODERATE, "PROX_COLOR_MODERATE / PW_YELLOW mismatch");
static_assert(PW_RED    == PROX_COLOR_CRITICAL, "PROX_COLOR_CRITICAL / PW_RED mismatch");
static const uint16_t PW_THREAT   = 0xF81F;  // magenta — confirmed tracker OUI

// ── Timing ───────────────────────────────────────────────────────────────────
static const uint16_t PROX_RADIO_MS  = 80;   // radio poll interval
static const uint16_t PROX_RENDER_MS = 120;  // TFT refresh interval

// ── Module interface ─────────────────────────────────────────────────────────
namespace ProximityWand {
    void setup();    // init hardware, draw initial screen
    void loop();     // non-blocking: radio poll + TFT render
    void teardown(); // release SPI bus before returning to menu
}

#endif // PROXIMITY_WAND_H
