// lib/logic/theme_logic.cpp
#include "theme_logic.h"

ColorSet buildColorSet(ThemeID id) {
    ColorSet cs = {};
    if (id == THEME_DARK) {
        cs.bg_base        = 0x18C3;
        cs.bg_surface     = 0x2969;
        cs.text_primary   = 0xF79D;
        cs.text_secondary = 0x9492;
        cs.accent_select  = 0xFEA0;
        cs.accent_border  = 0x5ACD;
        cs.status_ok      = 0x072E;
        cs.status_warn    = 0xFD80;
        cs.status_err     = 0xFA8A;
    } else {
        cs.bg_base        = 0xF79E;
        cs.bg_surface     = 0xEF5C;
        cs.text_primary   = 0x18C3;
        cs.text_secondary = 0x528A;
        cs.accent_select  = 0x01F0;
        cs.accent_border  = 0xAD55;
        cs.status_ok      = 0x03E6;
        cs.status_warn    = 0xE280;
        cs.status_err     = 0xB0E3;
    }
    cs.halehound_cyan     = cs.text_primary;
    cs.halehound_magenta  = cs.accent_select;
    cs.halehound_gunmetal = cs.bg_base;
    cs.halehound_green    = cs.status_ok;
    cs.halehound_black    = cs.bg_base;
    cs.halehound_hotpink  = cs.accent_select;
    cs.halehound_bright   = cs.accent_select;
    cs.halehound_violet   = cs.accent_border;
    cs.halehound_dark     = cs.bg_surface;
    cs.shreddy_teal       = cs.text_primary;
    cs.shreddy_pink       = cs.accent_select;
    cs.shreddy_black      = cs.bg_base;
    cs.shreddy_blue       = cs.accent_select;
    cs.shreddy_purple     = cs.accent_border;
    cs.shreddy_green      = cs.status_ok;
    cs.shreddy_gunmetal   = cs.bg_base;
    cs.orange             = cs.accent_select;
    cs.green              = cs.status_ok;
    cs.red                = cs.status_err;
    cs.dark_gray          = cs.bg_base;
    cs.gray               = cs.text_secondary;
    cs.light_gray         = cs.text_secondary;
    cs.white              = cs.text_primary;
    cs.black              = cs.bg_base;
    cs.blue               = cs.accent_select;
    return cs;
}

ThemeID validateThemeRaw(uint8_t raw) {
    return (raw == THEME_DARK || raw == THEME_LIGHT) ? (ThemeID)raw : THEME_DARK;
}
