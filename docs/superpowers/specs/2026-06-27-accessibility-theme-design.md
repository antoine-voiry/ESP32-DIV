# Design Spec: ESP32-DIV Accessibility-First Theme System

**Date:** 2026-06-27  
**Status:** Approved — implementation plan pending  

---

## Context

The ESP32-DIV firmware uses a "HaleHound" cyberpunk color palette — magenta/hotpink on near-black backgrounds — that causes visual fatigue and fails accessibility contrast thresholds for users with presbyopia or reduced contrast sensitivity. The goal is a runtime-switchable dual-theme system (Dark + Light) with WCAG-compliant contrast ratios, larger menu fonts, and a clean semantic color architecture — without touching the 460+ color references spread across the three large feature files (bluetooth.cpp, wifi.cpp, subghz.cpp).

---

## Architecture: Approach A — Global Variable Shim

Convert all `const uint16_t` color globals in `shared.h` from inline definitions (internal linkage) to `extern` declarations. Place the single-TU definitions + `applyTheme()` in a new `theme.cpp`. Legacy color names (SHREDDY_*, HALEHOUND_*, ORANGE, etc.) are reassigned to new semantic values on every theme switch — zero changes to feature files.

---

## Color Palette

### Semantic Tokens (new — use in all new code)

| Token | Role |
|-------|------|
| `BG_BASE` | Screen / full-bleed fill |
| `BG_SURFACE` | Card / button face |
| `TEXT_PRIMARY` | Body text, labels |
| `TEXT_SECONDARY` | Dim / inactive / secondary text |
| `ACCENT_SELECT` | Selected item, primary highlight |
| `ACCENT_BORDER` | Unselected border / left-edge stripe |
| `STATUS_OK` | Battery full, SD present, success |
| `STATUS_WARN` | Battery mid, temperature warning |
| `STATUS_ERR` | Battery low, error, critical |

### THEME_DARK — "Graphite" (default, indoor/low-light)

| Token | Hex | RGB565 | Contrast vs BG |
|-------|-----|--------|---------------|
| `BG_BASE` | `#1A1A1A` | `0x18C3` | — |
| `BG_SURFACE` | `#2D2D2D` | `0x2969` | — |
| `TEXT_PRIMARY` | `#F5F0E8` | `0xF79D` | **14.3:1** ✓ |
| `TEXT_SECONDARY` | `#909090` | `0x9492` | 5.8:1 ✓ |
| `ACCENT_SELECT` | `#FFD600` | `0xFEA0` | **11.5:1** ✓ |
| `ACCENT_BORDER` | `#5A5A6A` | `0x5ACD` | — |
| `STATUS_OK` | `#00E676` | `0x072E` | 9.2:1 ✓ |
| `STATUS_WARN` | `#FFB300` | `0xFD80` | 10.1:1 ✓ |
| `STATUS_ERR` | `#FF5252` | `0xFA8A` | 5.6:1 ✓ |

### THEME_LIGHT — "Paper" (outdoor / bright sunlight)

| Token | Hex | RGB565 | Contrast vs BG |
|-------|-----|--------|---------------|
| `BG_BASE` | `#F5F5F0` | `0xF79E` | — |
| `BG_SURFACE` | `#E8E8E0` | `0xEF5C` | — |
| `TEXT_PRIMARY` | `#1A1A1A` | `0x18C3` | **14.8:1** ✓ |
| `TEXT_SECONDARY` | `#505050` | `0x528A` | 7.4:1 ✓ |
| `ACCENT_SELECT` | `#003D82` | `0x01F0` | **9.7:1** ✓ |
| `ACCENT_BORDER` | `#AAAAAA` | `0xAD55` | — |
| `STATUS_OK` | `#007E33` | `0x03E6` | 7.8:1 ✓ |
| `STATUS_WARN` | `#E65100` | `0xE280` | 6.1:1 ✓ |
| `STATUS_ERR` | `#B71C1C` | `0xB0E3` | 7.2:1 ✓ |

### Legacy Name → Semantic Mapping (inside `applyTheme()`)

```
HALEHOUND_CYAN      → TEXT_PRIMARY
HALEHOUND_MAGENTA   → ACCENT_SELECT
HALEHOUND_GUNMETAL  → BG_BASE
HALEHOUND_GREEN     → STATUS_OK
HALEHOUND_BLACK     → BG_BASE
HALEHOUND_HOTPINK   → ACCENT_SELECT
HALEHOUND_BRIGHT    → ACCENT_SELECT
HALEHOUND_VIOLET    → ACCENT_BORDER
HALEHOUND_DARK      → BG_SURFACE
SHREDDY_TEAL        → TEXT_PRIMARY
SHREDDY_PINK        → ACCENT_SELECT
SHREDDY_BLACK       → BG_BASE
SHREDDY_BLUE        → ACCENT_SELECT
SHREDDY_PURPLE      → ACCENT_BORDER
SHREDDY_GREEN       → STATUS_OK
SHREDDY_GUNMETAL    → BG_BASE
ORANGE              → ACCENT_SELECT
GREEN               → STATUS_OK
RED                 → STATUS_ERR
DARK_GRAY           → BG_BASE
GRAY                → TEXT_SECONDARY
LIGHT_GRAY          → TEXT_SECONDARY
WHITE               → TEXT_PRIMARY
BLACK               → BG_BASE
BLUE                → ACCENT_SELECT
```

---

## Typography & Layout

### Font Strategy (TFT_eSPI built-in fonts only — no external .vlw files)

| Context | Font | setTextSize | Effective px |
|---------|------|-------------|-------------|
| Status bar text | Font 1 | 1 | 8px — unchanged |
| Grid menu labels | Font 2 | 1 | 16px — unchanged (button size constrains) |
| **Submenu items** | **Font 4** | **1** | **26px** — upgraded from Font 2 |
| Section headers (`drawHeader()`) | Font 4 | 1 | 26px |
| Notification overlays | Font 2 | 2 | 32px |

> **Rationale for Font 4 over setTextSize(2):** Font 4 at native 26px has better horizontal spacing than Font 2 doubled — avoids nearest-neighbor scale artifacts.

### Submenu Row Geometry

- Row height: **40px** (26px text + 7px top + 7px bottom padding)
- Available height with 24px status bar: `320 − 24 = 296px`
- Maximum visible rows: `296 / 40 = 7` — covers all current submenus (WiFi: 6, BT: 7, SubGHz: 5)
- Text vertical centering: `rowY + (40 - 26) / 2 = rowY + 7`

### Selection Bar Rendering

```cpp
// Selected row
tft.fillRoundRect(2, rowY, tft.width() - 4, 40, 4, ACCENT_SELECT);
tft.setTextColor(BG_BASE, ACCENT_SELECT);   // inverted for max contrast
tft.setTextFont(4);
tft.setCursor(iconOffset + 4, rowY + 7);
tft.print(label);

// Unselected row
tft.fillRect(0, rowY, tft.width(), 40, BG_BASE);
tft.fillRect(0, rowY, 4, 40, ACCENT_BORDER);   // 4px left-edge stripe
tft.setTextColor(TEXT_PRIMARY, BG_BASE);
tft.setTextFont(4);
tft.setCursor(iconOffset + 4, rowY + 7);
tft.print(label);
```

### Status Bar Height: 20 → 24px

- Reason: 16×16px icons at `y=4` get 4px breathing room top and bottom.
- Impact: menu content area start shifts from `y=20` to `y=24`. Update `Y_START = 34` (was 30) in `ESP32-DIV-clone.ino`.

---

## Implementation File Map

### Files Modified

| File | Change |
|------|--------|
| `shared.h` | All `const uint16_t X = Y` → `extern uint16_t X`; add 9 semantic token externs; add `ThemeID` enum; add `applyTheme()`, `loadThemeFromEEPROM()`, and `extern ThemeID currentThemeID` declarations |
| `eeprom_layout.h` | Add `#define EEPROM_THEME_ADDR 2`; add static_assert guard |
| `utils.cpp` | `drawStatusBar()`: `barHeight` 20→24; battery color → 3-tier STATUS_OK/STATUS_WARN/STATUS_ERR; icon/bar colors → semantic tokens |
| `ESP32-DIV-clone.ino` | Call `loadThemeFromEEPROM()` after `EEPROM.begin()` in `setup()`; update `Y_START = 34`; add theme-toggle entry to settings submenu |

### Files Created

| File | Contents |
|------|----------|
| `theme.cpp` | Definitions for all `uint16_t` color globals; `currentThemeID`; `applyTheme(ThemeID)` with both palette branches + full legacy remap; `loadThemeFromEEPROM()` with bounds check |
| `tools/verify_contrast.py` | Host-side WCAG contrast verification script |
| `tools/test_theme_logic.py` | Host-side unit test for `loadThemeFromEEPROM()` bounds logic |

### Files Untouched

`bluetooth.cpp`, `wifi.cpp`, `subghz.cpp` — all 460+ color references automatically inherit new values via remapped globals.

---

## Key Code Snippets

### `shared.h` (after)

```cpp
#pragma once
#include <Arduino.h>

enum ThemeID : uint8_t { THEME_DARK = 0, THEME_LIGHT = 1 };

// Semantic tokens — use in all new code
extern uint16_t BG_BASE, BG_SURFACE;
extern uint16_t TEXT_PRIMARY, TEXT_SECONDARY;
extern uint16_t ACCENT_SELECT, ACCENT_BORDER;
extern uint16_t STATUS_OK, STATUS_WARN, STATUS_ERR;

// Legacy names — remapped by applyTheme(), not used in new code
extern uint16_t HALEHOUND_MAGENTA, HALEHOUND_HOTPINK, HALEHOUND_BRIGHT;
extern uint16_t HALEHOUND_VIOLET, HALEHOUND_DARK, HALEHOUND_CYAN;
extern uint16_t HALEHOUND_BLACK, HALEHOUND_GUNMETAL, HALEHOUND_GREEN;
extern uint16_t SHREDDY_TEAL, SHREDDY_PINK, SHREDDY_BLACK;
extern uint16_t SHREDDY_BLUE, SHREDDY_PURPLE, SHREDDY_GREEN, SHREDDY_GUNMETAL;
extern uint16_t ORANGE, GRAY, BLUE, RED, GREEN, BLACK, WHITE, LIGHT_GRAY, DARK_GRAY;

#define TFT_DARKBLUE  0x3166
#define TFTWHITE      0xFFFF
#define TFT_GRAY      0x8410
#define SELECTED_ICON_COLOR ACCENT_SELECT

void applyTheme(ThemeID id);
ThemeID loadThemeFromEEPROM();
extern ThemeID currentThemeID;

// ... existing extern bool flags unchanged
```

### `theme.cpp` (complete)

```cpp
#include "shared.h"
#include <EEPROM.h>
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

void applyTheme(ThemeID id) {
    if (id == THEME_DARK) {
        BG_BASE        = 0x18C3;  // #1A1A1A
        BG_SURFACE     = 0x2969;  // #2D2D2D
        TEXT_PRIMARY   = 0xF79D;  // #F5F0E8 warm white
        TEXT_SECONDARY = 0x9492;  // #909090
        ACCENT_SELECT  = 0xFEA0;  // #FFD600 amber
        ACCENT_BORDER  = 0x5ACD;  // #5A5A6A
        STATUS_OK      = 0x072E;  // #00E676
        STATUS_WARN    = 0xFD80;  // #FFB300
        STATUS_ERR     = 0xFA8A;  // #FF5252
    } else {
        BG_BASE        = 0xF79E;  // #F5F5F0
        BG_SURFACE     = 0xEF5C;  // #E8E8E0
        TEXT_PRIMARY   = 0x18C3;  // #1A1A1A
        TEXT_SECONDARY = 0x528A;  // #505050
        ACCENT_SELECT  = 0x01F0;  // #003D82 deep blue
        ACCENT_BORDER  = 0xAD55;  // #AAAAAA
        STATUS_OK      = 0x03E6;  // #007E33
        STATUS_WARN    = 0xE280;  // #E65100
        STATUS_ERR     = 0xB0E3;  // #B71C1C
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

    currentThemeID = id;
    EEPROM.write(EEPROM_THEME_ADDR, (uint8_t)id);
    EEPROM.commit();
}

ThemeID loadThemeFromEEPROM() {
    ThemeID id = (ThemeID)EEPROM.read(EEPROM_THEME_ADDR);
    if (id != THEME_DARK && id != THEME_LIGHT) id = THEME_DARK;
    applyTheme(id);
    return id;
}
```

### `eeprom_layout.h` addition

```cpp
// ── Theme (theme.cpp) ─────────────────────────────────────────────────────
#define EEPROM_THEME_ADDR    2   // 1 byte: ThemeID (0=dark, 1=light)
static_assert(EEPROM_THEME_ADDR < CP_CRED_ADDR,
    "EEPROM COLLISION: Theme byte overlaps CaptivePortal region");
```

### `setup()` addition in `.ino`

```cpp
EEPROM.begin(EEPROM_TOTAL_SIZE);
loadThemeFromEEPROM();   // before any tft.* calls
```

### Settings menu theme toggle

```cpp
ThemeID next = (currentThemeID == THEME_DARK) ? THEME_LIGHT : THEME_DARK;
applyTheme(next);
// Return to main menu — displayMenu() redraws all colors automatically
```

### `drawStatusBar()` key changes (`utils.cpp`)

```cpp
int barHeight = 24;  // was 20

uint16_t batteryColor = (batteryPercentage > 50) ? STATUS_OK
                      : (batteryPercentage > 20) ? STATUS_WARN : STATUS_ERR;
tft.fillRoundRect(x + 2, y + 2, batteryLevelWidth, 6, 1, batteryColor);
tft.fillRect(0, 0, tft.width(), barHeight, BG_SURFACE);
tft.drawRoundRect(x, y, 22, 10, 2, ACCENT_BORDER);
tft.fillRect(x + 22, y + 3, 2, 4, ACCENT_BORDER);
tft.setTextColor(TEXT_PRIMARY, BG_SURFACE);
// WiFi bars
tft.fillRoundRect(barX, wifiY - barH, barWidth, barH, 1,
    (wifiStrength > i * 25) ? STATUS_OK : ACCENT_BORDER);
```

---

## Testing Strategy

### Host-Side: Contrast Verification (`tools/verify_contrast.py`)

Validates every RGB565 palette value against WCAG 2.1 thresholds before any flash. No hardware required. Primary automated gate. Run: `python3 tools/verify_contrast.py`

Checks all text/background pairs in both themes: TEXT_PRIMARY, TEXT_SECONDARY, ACCENT_SELECT, STATUS_OK/WARN/ERR on BG_BASE, plus BG_BASE on ACCENT_SELECT (inverted selected row). Exits non-zero on any failure.

### Host-Side: Logic Test (`tools/test_theme_logic.py`)

Validates `loadThemeFromEEPROM()` bounds check: byte 0 → DARK, byte 1 → LIGHT, byte 0xFF → DARK (default), byte 2 → DARK (out-of-range).

### On-Device Verification

1. Compile — zero new warnings expected from extern refactor.
2. First boot — EEPROM byte 2 = 0xFF (virgin), defaults to THEME_DARK, no panic.
3. Theme toggle — Dark → Light → Dark in settings; display redraws both times.
4. Smoke test — Navigate all three feature module submenus in both themes; no invisible text.
5. Status bar — Battery fill transitions STATUS_OK → STATUS_WARN → STATUS_ERR; 24px bar does not clip menu content.

---

## Accessibility Verification Checklist

Apply to every new screen before committing:

1. **Contrast gate** — Every text/BG pair: ≥ 4.5:1 body, ≥ 7:1 status/critical. Verify via W3C luminance formula or `verify_contrast.py`.
2. **Token-only colors** — All `tft.setTextColor()` / `tft.fillRect()` / `tft.drawRoundRect()` calls use named tokens, never raw hex literals.
3. **Dual-theme test** — `applyTheme(THEME_DARK)` then `applyTheme(THEME_LIGHT)`; no leftover artifacts in either pass.
4. **Touch-target floor** — Interactive rows/buttons: ≥ 40px tall, ≥ 44px wide.
5. **Font floor** — Minimum `setTextFont(2)` (16px) for user-facing text; `setTextFont(1)` only for debug/overflow.

---

## Commit Strategy

Single atomic commit after all five on-device verification steps pass. All files land together: `theme.cpp`, `shared.h`, `eeprom_layout.h`, `utils.cpp`, `ESP32-DIV-clone.ino`, `tools/verify_contrast.py`, `tools/test_theme_logic.py`.

```
feat: accessibility-first dual-theme system (Graphite / Paper)

- Refactor shared.h color globals from const to extern; single-TU
  definitions in new theme.cpp with applyTheme(THEME_DARK|THEME_LIGHT)
- Add 9 semantic color tokens (BG_BASE, TEXT_PRIMARY, ACCENT_SELECT, etc.)
  with WCAG-verified contrast: ≥4.5:1 body, ≥7:1 status indicators
- Persist active theme (1 byte) at EEPROM_THEME_ADDR=2; auto-loads on boot
- Status bar height 20→24px; battery fill 3-tier semantic color
- Submenu rows: Font 4 (26px), 40px row height, rounded selection bar
- tools/verify_contrast.py: host-side WCAG contrast gate (no hardware needed)

Zero changes to bluetooth.cpp / wifi.cpp / subghz.cpp.
```
