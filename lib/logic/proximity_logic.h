// lib/logic/proximity_logic.h
// Pure C++ — no Arduino or hardware headers.
// Testable on native host without hardware.
#ifndef PROXIMITY_LOGIC_H
#define PROXIMITY_LOGIC_H

#include <stdint.h>
#include <string.h>

// ── RSSI zone classification ──────────────────────────────────────────────────
// Maps raw dBm to a 4-level proximity zone for UI color and label decisions.
enum RssiZone : uint8_t {
    ZONE_NONE     = 0,   // < -90 dBm  — noise floor, no usable signal
    ZONE_LOW      = 1,   // -90 to -70 — distant
    ZONE_MODERATE = 2,   // -70 to -50 — approaching
    ZONE_CRITICAL = 3,   // ≥  -50 dBm — close range
};

inline RssiZone classifyRssi(int8_t rssi) {
    if (rssi < -90) return ZONE_NONE;
    if (rssi < -70) return ZONE_LOW;
    if (rssi < -50) return ZONE_MODERATE;
    return ZONE_CRITICAL;
}

// ── Bar-graph pixel width ─────────────────────────────────────────────────────
// Maps [-100, -30] dBm → [0, displayWidth] pixels.
// Values outside the range are clamped; no float arithmetic.
inline uint16_t rssiToBarWidth(int8_t rssi, uint16_t displayWidth) {
    const int8_t RSSI_MIN = -100;
    const int8_t RSSI_MAX = -30;
    if (rssi <= RSSI_MIN) return 0;
    if (rssi >= RSSI_MAX) return displayWidth;
    return static_cast<uint16_t>(
        (static_cast<int32_t>(rssi - RSSI_MIN) * displayWidth) / (RSSI_MAX - RSSI_MIN)
    );
}

// ── EWMA RSSI smoothing ───────────────────────────────────────────────────────
// α ≈ 0.3 in fixed-point (α×16 = 5, (1-α)×16 = 11): smoothed_new = (5*sample + 11*smoothed) / 16.
// `smoothed` holds the running average in plain dBm units (same scale as newSample) —
// it is NOT pre-scaled by 16; only the alpha/(1-alpha) weights are fixed-point.
// On first call, initialize smoothed = (int16_t)firstSample.
inline int8_t ewmaRssi(volatile int16_t& smoothed, int8_t newSample) {
    smoothed = static_cast<int16_t>(
        (5 * static_cast<int16_t>(newSample) + 11 * smoothed) >> 4
    );
    return static_cast<int8_t>(smoothed);
}

// ── Proximity zone color constants (RGB565) ───────────────────────────────────
// Hex values match PW_DIM / PW_GREEN / PW_YELLOW / PW_RED in proximity_wand.h.
// Defined here so zoneColor() needs no TFT or Arduino headers.
static constexpr uint16_t PROX_COLOR_NONE     = 0x4208;  // dark grey — no signal
static constexpr uint16_t PROX_COLOR_LOW      = 0x07E0;  // green — distant
static constexpr uint16_t PROX_COLOR_MODERATE = 0xFFE0;  // yellow — approaching
static constexpr uint16_t PROX_COLOR_CRITICAL = 0xF800;  // red — close range

// ── BLE hit-count → synthetic dBm proxy ──────────────────────────────────────
// Maps packet hit-count in one PROX_RADIO_MS window to a synthetic dBm value.
// 0 hits = -100 dBm (noise floor); 5+ hits = -40 dBm (very close).
inline int8_t hitsToRssiProxy(uint8_t hits) {
    if (hits == 0) return -100;
    if (hits == 1) return -80;
    if (hits == 2) return -65;
    if (hits == 3) return -55;
    if (hits == 4) return -47;
    return -40;
}

// ── Zone → display color ──────────────────────────────────────────────────────
inline uint16_t zoneColor(RssiZone z) {
    switch (z) {
        case ZONE_NONE:     return PROX_COLOR_NONE;
        case ZONE_LOW:      return PROX_COLOR_LOW;
        case ZONE_MODERATE: return PROX_COLOR_MODERATE;
        case ZONE_CRITICAL: return PROX_COLOR_CRITICAL;
    }
    return PROX_COLOR_NONE;
}

// ── Zone → display label ──────────────────────────────────────────────────────
inline const char* zoneLabel(RssiZone z) {
    switch (z) {
        case ZONE_NONE:     return "NO SIGNAL";
        case ZONE_LOW:      return "DISTANT";
        case ZONE_MODERATE: return "APPROACHING";
        case ZONE_CRITICAL: return "CLOSE RANGE";
    }
    return "";
}

// ── OUI lookup ────────────────────────────────────────────────────────────────
// Walks the OUI_TABLE (defined in proximity_wand.h, passed by caller).
// On ESP32, PROGMEM const arrays live in flash; memcmp accesses them normally.
//
// OuiEntry layout (packed, total 20 bytes):
//   uint8_t  oui[3]     — 3 bytes at offset 0
//   char     vendor[16] — 16 bytes at offset 3
//   bool     threat     — 1 byte  at offset 19
//
// Returns true and fills vendorOut[16] + *threatOut when found.
// vendorOut must be at least 16 bytes; always NUL-terminated on return.
struct OuiEntry {
    uint8_t oui[3];
    char    vendor[16];
    bool    threat;
} __attribute__((packed));

inline bool lookupOui(
    const uint8_t*   mac3,
    const OuiEntry*  table,
    uint8_t          entryCount,
    char*            vendorOut,
    bool*            threatOut
) {
    for (uint8_t i = 0; i < entryCount; ++i) {
        if (table[i].oui[0] == mac3[0] &&
            table[i].oui[1] == mac3[1] &&
            table[i].oui[2] == mac3[2]) {
            memcpy(vendorOut, table[i].vendor, 16);
            vendorOut[15] = '\0';
            *threatOut = table[i].threat;
            return true;
        }
    }
    vendorOut[0] = '\0';
    *threatOut = false;
    return false;
}

#endif // PROXIMITY_LOGIC_H
