// src/theme.cpp
#include "shared.h"
#include "hal/hal_globals.h"
#include "hal/IEeprom.h"
#include "theme_logic.h"
#include "eeprom_layout.h"

ThemeID currentThemeID = THEME_DARK;

uint16_t BG_BASE, BG_SURFACE, TEXT_PRIMARY, TEXT_SECONDARY;
uint16_t ACCENT_SELECT, ACCENT_BORDER, STATUS_OK, STATUS_WARN, STATUS_ERR;
uint16_t HALEHOUND_MAGENTA, HALEHOUND_HOTPINK, HALEHOUND_BRIGHT;
uint16_t HALEHOUND_VIOLET, HALEHOUND_DARK, HALEHOUND_CYAN;
uint16_t HALEHOUND_BLACK, HALEHOUND_GUNMETAL, HALEHOUND_GREEN;
uint16_t SHREDDY_TEAL, SHREDDY_PINK, SHREDDY_BLACK, SHREDDY_BLUE;
uint16_t SHREDDY_PURPLE, SHREDDY_GREEN, SHREDDY_GUNMETAL;
uint16_t ORANGE, GRAY, BLUE, RED, GREEN, BLACK, WHITE, LIGHT_GRAY, DARK_GRAY;

static void applyColorSet(const ColorSet& cs) {
    BG_BASE            = cs.bg_base;
    BG_SURFACE         = cs.bg_surface;
    TEXT_PRIMARY       = cs.text_primary;
    TEXT_SECONDARY     = cs.text_secondary;
    ACCENT_SELECT      = cs.accent_select;
    ACCENT_BORDER      = cs.accent_border;
    STATUS_OK          = cs.status_ok;
    STATUS_WARN        = cs.status_warn;
    STATUS_ERR         = cs.status_err;
    HALEHOUND_MAGENTA  = cs.halehound_magenta;
    HALEHOUND_HOTPINK  = cs.halehound_hotpink;
    HALEHOUND_BRIGHT   = cs.halehound_bright;
    HALEHOUND_VIOLET   = cs.halehound_violet;
    HALEHOUND_DARK     = cs.halehound_dark;
    HALEHOUND_CYAN     = cs.halehound_cyan;
    HALEHOUND_BLACK    = cs.halehound_black;
    HALEHOUND_GUNMETAL = cs.halehound_gunmetal;
    HALEHOUND_GREEN    = cs.halehound_green;
    SHREDDY_TEAL       = cs.shreddy_teal;
    SHREDDY_PINK       = cs.shreddy_pink;
    SHREDDY_BLACK      = cs.shreddy_black;
    SHREDDY_BLUE       = cs.shreddy_blue;
    SHREDDY_PURPLE     = cs.shreddy_purple;
    SHREDDY_GREEN      = cs.shreddy_green;
    SHREDDY_GUNMETAL   = cs.shreddy_gunmetal;
    ORANGE             = cs.orange;
    GRAY               = cs.gray;
    BLUE               = cs.blue;
    RED                = cs.red;
    GREEN              = cs.green;
    BLACK              = cs.black;
    WHITE              = cs.white;
    LIGHT_GRAY         = cs.light_gray;
    DARK_GRAY          = cs.dark_gray;
}

void applyTheme(ThemeID id) {
    applyColorSet(buildColorSet(id));
    currentThemeID = id;
    gEeprom->write(EEPROM_THEME_ADDR, (uint8_t)id);
    gEeprom->commit();
}

ThemeID loadThemeFromEEPROM() {
    ThemeID id = validateThemeRaw(gEeprom->read(EEPROM_THEME_ADDR));
    applyColorSet(buildColorSet(id));
    currentThemeID = id;
    return id;
}
