#ifndef SHARED_H
#define SHARED_H

#include <Arduino.h>

// ── Theme control ──────────────────────────────────────────────────────────
enum ThemeID : uint8_t { THEME_DARK = 0, THEME_LIGHT = 1 };

void applyTheme(ThemeID id);
ThemeID loadThemeFromEEPROM();
extern ThemeID currentThemeID;

// ── Semantic tokens (use these in all new code) ───────────────────────────
extern uint16_t BG_BASE;        // screen fill
extern uint16_t BG_SURFACE;     // card / status bar background
extern uint16_t TEXT_PRIMARY;   // body text
extern uint16_t TEXT_SECONDARY; // dim / inactive text
extern uint16_t ACCENT_SELECT;  // selected item highlight
extern uint16_t ACCENT_BORDER;  // unselected border / left-edge stripe
extern uint16_t STATUS_OK;      // battery full, SD present, success
extern uint16_t STATUS_WARN;    // battery mid, temperature caution
extern uint16_t STATUS_ERR;     // battery low, error, critical

// ── Legacy names — remapped by applyTheme(), do not use in new code ───────
extern uint16_t HALEHOUND_MAGENTA;
extern uint16_t HALEHOUND_HOTPINK;
extern uint16_t HALEHOUND_BRIGHT;
extern uint16_t HALEHOUND_VIOLET;
extern uint16_t HALEHOUND_DARK;
extern uint16_t HALEHOUND_CYAN;
extern uint16_t HALEHOUND_BLACK;
extern uint16_t HALEHOUND_GUNMETAL;
extern uint16_t HALEHOUND_GREEN;
extern uint16_t SHREDDY_TEAL;
extern uint16_t SHREDDY_PINK;
extern uint16_t SHREDDY_BLACK;
extern uint16_t SHREDDY_BLUE;
extern uint16_t SHREDDY_PURPLE;
extern uint16_t SHREDDY_GREEN;
extern uint16_t SHREDDY_GUNMETAL;
extern uint16_t ORANGE;
extern uint16_t GRAY;
extern uint16_t BLUE;
extern uint16_t RED;
extern uint16_t GREEN;
extern uint16_t BLACK;
extern uint16_t WHITE;
extern uint16_t LIGHT_GRAY;
extern uint16_t DARK_GRAY;

// ── Compile-time color constants (not theme-dependent) ────────────────────
#define TFT_DARKBLUE      0x3166
#define TFT_LIGHTBLUE     0x051F
#define TFTWHITE          0xFFFF
#define TFT_GRAY          0x8410
#define SELECTED_ICON_COLOR ACCENT_SELECT

// ── UI state flags ────────────────────────────────────────────────────────
void displaySubmenu();

extern bool in_sub_menu;
extern bool feature_active;
extern bool submenu_initialized;
extern bool is_main_menu;
extern bool feature_exit_requested;

#endif // SHARED_H
