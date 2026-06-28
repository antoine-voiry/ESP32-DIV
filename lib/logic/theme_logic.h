// lib/logic/theme_logic.h
#pragma once
#include <stdint.h>
#include "theme_types.h"

struct ColorSet {
    uint16_t bg_base, bg_surface;
    uint16_t text_primary, text_secondary;
    uint16_t accent_select, accent_border;
    uint16_t status_ok, status_warn, status_err;
    uint16_t halehound_magenta, halehound_hotpink, halehound_bright;
    uint16_t halehound_violet, halehound_dark, halehound_cyan;
    uint16_t halehound_black, halehound_gunmetal, halehound_green;
    uint16_t shreddy_teal, shreddy_pink, shreddy_black, shreddy_blue;
    uint16_t shreddy_purple, shreddy_green, shreddy_gunmetal;
    uint16_t orange, gray, blue, red, green, black, white, light_gray, dark_gray;
};

ColorSet buildColorSet(ThemeID id);
ThemeID  validateThemeRaw(uint8_t raw);
