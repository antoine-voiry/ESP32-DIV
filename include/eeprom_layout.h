#pragma once

#define EEPROM_TOTAL_SIZE     1500

// ── Theme (theme.cpp) ─────────────────────────────────────────────────────
#define EEPROM_THEME_ADDR    2   // 1 byte: ThemeID (0=dark, 1=light)

// ── CaptivePortal (wifi.cpp) ──────────────────────────────────────────────
#define CP_CRED_ADDR         32
#define CP_CRED_SIZE         64
#define CP_MAX_CREDS         20
#define CP_COUNT_ADDR        1248
// Credentials occupy bytes 32–1311.

// ── SubGHz (subghz.cpp) ──────────────────────────────────────────────────
#define SUBGHZ_ADDR_VALUE    1320   // 4 bytes  (unsigned long)
#define SUBGHZ_ADDR_BITLEN   1324   // 2 bytes  (int)
#define SUBGHZ_ADDR_PROTO    1326   // 2 bytes  (int)
#define SUBGHZ_ADDR_FREQ     1328   // 4 bytes  (uint32_t)
// Profile count is at byte 0 (ADDR_PROFILE_COUNT = 0, unchanged).
#define SUBGHZ_ADDR_PROFILE_START  1340   // 4 profiles × 32 bytes = 128 bytes
#define SUBGHZ_MAX_PROFILES        4

// ── Compile-time overlap guard ────────────────────────────────────────────
static_assert(SUBGHZ_ADDR_VALUE > (CP_CRED_ADDR + CP_MAX_CREDS * CP_CRED_SIZE),
    "EEPROM COLLISION: SubGHz region overlaps CaptivePortal credentials");
static_assert(SUBGHZ_ADDR_PROFILE_START + SUBGHZ_MAX_PROFILES * 32 < EEPROM_TOTAL_SIZE,
    "EEPROM OVERFLOW: SubGHz profiles exceed EEPROM_TOTAL_SIZE");
static_assert(EEPROM_THEME_ADDR < CP_CRED_ADDR,
    "EEPROM COLLISION: Theme byte overlaps CaptivePortal region");
