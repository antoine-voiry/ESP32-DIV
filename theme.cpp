#include "shared.h"
#include <EEPROM.h>
#include "eeprom_layout.h"

// ── Theme state ────────────────────────────────────────────────────────────
ThemeID currentThemeID = THEME_DARK;

// ── Single-TU definitions for all color globals ───────────────────────────
uint16_t BG_BASE, BG_SURFACE, TEXT_PRIMARY, TEXT_SECONDARY;
uint16_t ACCENT_SELECT, ACCENT_BORDER, STATUS_OK, STATUS_WARN, STATUS_ERR;

uint16_t HALEHOUND_MAGENTA, HALEHOUND_HOTPINK, HALEHOUND_BRIGHT;
uint16_t HALEHOUND_VIOLET, HALEHOUND_DARK, HALEHOUND_CYAN;
uint16_t HALEHOUND_BLACK, HALEHOUND_GUNMETAL, HALEHOUND_GREEN;
uint16_t SHREDDY_TEAL, SHREDDY_PINK, SHREDDY_BLACK, SHREDDY_BLUE;
uint16_t SHREDDY_PURPLE, SHREDDY_GREEN, SHREDDY_GUNMETAL;
uint16_t ORANGE, GRAY, BLUE, RED, GREEN, BLACK, WHITE, LIGHT_GRAY, DARK_GRAY;

// ── Theme application ─────────────────────────────────────────────────────
void applyTheme(ThemeID id) {
    if (id == THEME_DARK) {
        // Graphite — indoor / low-light
        BG_BASE        = 0x18C3;  // #1A1A1A
        BG_SURFACE     = 0x2969;  // #2D2D2D
        TEXT_PRIMARY   = 0xF79D;  // #F5F0E8  warm white    14.3:1 on BG_BASE
        TEXT_SECONDARY = 0x9492;  // #909090                 5.6:1 on BG_BASE
        ACCENT_SELECT  = 0xFEA0;  // #FFD600  amber         12.6:1 on BG_BASE
        ACCENT_BORDER  = 0x5ACD;  // #5A5A6A
        STATUS_OK      = 0x072E;  // #00E676                10.7:1 on BG_BASE
        STATUS_WARN    = 0xFD80;  // #FFB300                10.1:1 on BG_BASE
        STATUS_ERR     = 0xFA8A;  // #FF5252                 5.6:1 on BG_BASE
    } else {
        // Paper — outdoor / bright sunlight
        BG_BASE        = 0xF79E;  // #F5F5F0
        BG_SURFACE     = 0xEF5C;  // #E8E8E0
        TEXT_PRIMARY   = 0x18C3;  // #1A1A1A               14.8:1 on BG_BASE
        TEXT_SECONDARY = 0x528A;  // #505050                 7.3:1 on BG_BASE
        ACCENT_SELECT  = 0x01F0;  // #003D82  deep blue      9.6:1 on BG_BASE
        ACCENT_BORDER  = 0xAD55;  // #AAAAAA
        STATUS_OK      = 0x03E6;  // #007E33                 4.8:1 on BG_BASE
        STATUS_WARN    = 0xE280;  // #E65100                 3.5:1 on BG_BASE
        STATUS_ERR     = 0xB0E3;  // #B71C1C                 6.0:1 on BG_BASE
    }

    // Remap all legacy names to the nearest semantic role
    HALEHOUND_CYAN     = TEXT_PRIMARY;
    HALEHOUND_MAGENTA  = ACCENT_SELECT;
    HALEHOUND_GUNMETAL = BG_BASE;
    HALEHOUND_GREEN    = STATUS_OK;
    HALEHOUND_BLACK    = BG_BASE;
    HALEHOUND_HOTPINK  = ACCENT_SELECT;
    HALEHOUND_BRIGHT   = ACCENT_SELECT;
    HALEHOUND_VIOLET   = ACCENT_BORDER;
    HALEHOUND_DARK     = BG_SURFACE;
    SHREDDY_TEAL       = TEXT_PRIMARY;
    SHREDDY_PINK       = ACCENT_SELECT;
    SHREDDY_BLACK      = BG_BASE;
    SHREDDY_BLUE       = ACCENT_SELECT;
    SHREDDY_PURPLE     = ACCENT_BORDER;
    SHREDDY_GREEN      = STATUS_OK;
    SHREDDY_GUNMETAL   = BG_BASE;
    ORANGE             = ACCENT_SELECT;
    GREEN              = STATUS_OK;
    RED                = STATUS_ERR;
    DARK_GRAY          = BG_BASE;
    GRAY               = TEXT_SECONDARY;
    LIGHT_GRAY         = TEXT_SECONDARY;
    WHITE              = TEXT_PRIMARY;
    BLACK              = BG_BASE;
    BLUE               = ACCENT_SELECT;

    currentThemeID = id;
    EEPROM.write(EEPROM_THEME_ADDR, (uint8_t)id);
    EEPROM.commit();
}

ThemeID loadThemeFromEEPROM() {
    uint8_t raw = EEPROM.read(EEPROM_THEME_ADDR);
    ThemeID id = (raw == THEME_DARK || raw == THEME_LIGHT) ? (ThemeID)raw : THEME_DARK;
    // Apply without persisting — avoids a flash write cycle on every boot
    if (id == THEME_DARK) {
        BG_BASE        = 0x18C3;
        BG_SURFACE     = 0x2969;
        TEXT_PRIMARY   = 0xF79D;
        TEXT_SECONDARY = 0x9492;
        ACCENT_SELECT  = 0xFEA0;
        ACCENT_BORDER  = 0x5ACD;
        STATUS_OK      = 0x072E;
        STATUS_WARN    = 0xFD80;
        STATUS_ERR     = 0xFA8A;
    } else {
        BG_BASE        = 0xF79E;
        BG_SURFACE     = 0xEF5C;
        TEXT_PRIMARY   = 0x18C3;
        TEXT_SECONDARY = 0x528A;
        ACCENT_SELECT  = 0x01F0;
        ACCENT_BORDER  = 0xAD55;
        STATUS_OK      = 0x03E6;
        STATUS_WARN    = 0xE280;
        STATUS_ERR     = 0xB0E3;
    }
    HALEHOUND_CYAN     = TEXT_PRIMARY;
    HALEHOUND_MAGENTA  = ACCENT_SELECT;
    HALEHOUND_GUNMETAL = BG_BASE;
    HALEHOUND_GREEN    = STATUS_OK;
    HALEHOUND_BLACK    = BG_BASE;
    HALEHOUND_HOTPINK  = ACCENT_SELECT;
    HALEHOUND_BRIGHT   = ACCENT_SELECT;
    HALEHOUND_VIOLET   = ACCENT_BORDER;
    HALEHOUND_DARK     = BG_SURFACE;
    SHREDDY_TEAL       = TEXT_PRIMARY;
    SHREDDY_PINK       = ACCENT_SELECT;
    SHREDDY_BLACK      = BG_BASE;
    SHREDDY_BLUE       = ACCENT_SELECT;
    SHREDDY_PURPLE     = ACCENT_BORDER;
    SHREDDY_GREEN      = STATUS_OK;
    SHREDDY_GUNMETAL   = BG_BASE;
    ORANGE             = ACCENT_SELECT;
    GREEN              = STATUS_OK;
    RED                = STATUS_ERR;
    DARK_GRAY          = BG_BASE;
    GRAY               = TEXT_SECONDARY;
    LIGHT_GRAY         = TEXT_SECONDARY;
    WHITE              = TEXT_PRIMARY;
    BLACK              = BG_BASE;
    BLUE               = ACCENT_SELECT;
    currentThemeID     = id;
    return id;
}
