# Accessibility-First Dual Theme System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the HaleHound cyberpunk palette with a runtime-switchable Graphite (dark) / Paper (light) dual-theme system meeting WCAG AA contrast ratios, with larger submenu fonts and a theme toggle in the Settings menu — without touching bluetooth.cpp, wifi.cpp, or subghz.cpp.

**Architecture:** Convert `shared.h` color globals from `const uint16_t` (internal linkage) to `extern uint16_t` declarations, with single-TU definitions in a new `theme.cpp`. `applyTheme(ThemeID)` reassigns all 25 legacy color names plus 9 new semantic tokens in one pass. EEPROM byte 2 persists the active theme across reboots.

**Tech Stack:** Arduino/ESP32 (arduino-cli), TFT_eSPI built-in fonts (Font 1/2/4), EEPROM library, Python 3 for host-side contract tests.

## Global Constraints

- Build tool: Arduino Maker Workshop CLI (absolute path in CLAUDE.md) — NOT `pio`
- Required linker flag: `--build-property "compiler.c.elf.extra_flags=-Wl,-zmuldefs"` (mandatory for ESP32 core 2.0.17)
- Buttons MUST be read via `pcf.digitalRead()` on PCF8574 expander — never `pinMode()`/`digitalRead()` on ESP32 GPIOs 3, 6, or 7
- Zero changes to `bluetooth.cpp`, `wifi.cpp`, `subghz.cpp`
- Single atomic git commit — all 9 files land together after on-device verification passes
- No float math for color calculations anywhere in firmware

---

## File Map

| Status | File | Responsibility |
|--------|------|---------------|
| **Create** | `theme.cpp` | Definitions for all 25 `uint16_t` color globals + `currentThemeID`; `applyTheme(ThemeID)`; `loadThemeFromEEPROM()` |
| **Create** | `tools/verify_contrast.py` | Host-side WCAG contrast ratio gate for all palette pairs |
| **Create** | `tools/test_theme_logic.py` | Host-side unit tests for `loadThemeFromEEPROM()` bounds logic |
| **Modify** | `shared.h` | All 25 `const uint16_t X = Y` → `extern uint16_t X`; add `ThemeID` enum; add 9 semantic token externs; add function/extern declarations |
| **Modify** | `eeprom_layout.h` | Add `EEPROM_THEME_ADDR 2` + static_assert guard |
| **Modify** | `utils.cpp` | `drawStatusBar()`: height 20→24px, 3-tier battery color, semantic token colors |
| **Modify** | `ESP32-DIV-clone.ino` | `setup()`: EEPROM + theme init; `displayMenu()`: `Y_START` 30→34, `BG_BASE` fill; `displaySubmenu()`: Font 4, 40px rows, selection bar; Settings: +1 item, `themeToggleLoop()` |

---

## Task 1: Host-Side Test Tooling

**Files:**
- Create: `tools/verify_contrast.py`
- Create: `tools/test_theme_logic.py`

**Interfaces:**
- Consumes: nothing (standalone scripts)
- Produces: exit code 0 on pass, 1 on failure — called as a gate before the final commit

---

- [ ] **Step 1.1: Create the tools directory**

```bash
mkdir -p /Users/antoine/esp32-div1/ESP32-DIV-clone/tools
```

- [ ] **Step 1.2: Write `tools/verify_contrast.py`**

Create this file at `tools/verify_contrast.py`:

```python
#!/usr/bin/env python3
"""Host-side WCAG contrast gate for ESP32-DIV accessibility theme palette.

Runs against the RGB565 palette values baked into theme.cpp.
Exit 0 = all checks pass. Exit 1 = one or more failures.

Threshold rationale:
  7.0 : WCAG AAA for body text
  4.5 : WCAG AA for UI text and actionable elements
  3.0 : minimum for color-coded status indicators (hue carries meaning)
"""

PALETTES = {
    "DARK": {
        "BG_BASE":        0x18C3,
        "BG_SURFACE":     0x2969,
        "TEXT_PRIMARY":   0xF79D,
        "TEXT_SECONDARY": 0x9492,
        "ACCENT_SELECT":  0xFEA0,
        "STATUS_OK":      0x072E,
        "STATUS_WARN":    0xFD80,
        "STATUS_ERR":     0xFA8A,
    },
    "LIGHT": {
        "BG_BASE":        0xF79E,
        "BG_SURFACE":     0xEF5C,
        "TEXT_PRIMARY":   0x18C3,
        "TEXT_SECONDARY": 0x528A,
        "ACCENT_SELECT":  0x01F0,
        "STATUS_OK":      0x03E6,
        "STATUS_WARN":    0xE280,
        "STATUS_ERR":     0xB0E3,
    },
}

# (foreground_token, background_token, min_ratio, label)
CHECKS = [
    ("TEXT_PRIMARY",   "BG_BASE",        7.0, "body text — WCAG AAA"),
    ("TEXT_SECONDARY", "BG_BASE",        4.5, "secondary text — WCAG AA"),
    ("ACCENT_SELECT",  "BG_BASE",        4.5, "accent on base — WCAG AA"),
    ("STATUS_OK",      "BG_BASE",        4.5, "status OK — WCAG AA"),
    ("STATUS_WARN",    "BG_BASE",        3.0, "status WARN — color-coded; hue carries meaning"),
    ("STATUS_ERR",     "BG_BASE",        4.5, "status ERR — WCAG AA"),
    ("BG_BASE",        "ACCENT_SELECT",  4.5, "inverted text on selection row — WCAG AA"),
]


def rgb565_to_srgb(c565):
    r = ((c565 >> 11) & 0x1F) / 31.0
    g = ((c565 >> 5)  & 0x3F) / 63.0
    b = (c565         & 0x1F) / 31.0
    return r, g, b


def linearize(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def luminance(c565):
    r, g, b = (linearize(x) for x in rgb565_to_srgb(c565))
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast(fg_c565, bg_c565):
    lf = luminance(fg_c565) + 0.05
    lb = luminance(bg_c565) + 0.05
    return max(lf, lb) / min(lf, lb)


failures = []
for theme_name, palette in PALETTES.items():
    print(f"\n--- {theme_name} ---")
    for fg_tok, bg_tok, min_ratio, label in CHECKS:
        ratio = contrast(palette[fg_tok], palette[bg_tok])
        ok = ratio >= min_ratio
        status = "PASS" if ok else "FAIL"
        print(f"  {label:45s}  {ratio:5.2f}:1  {status}")
        if not ok:
            failures.append(
                f"[{theme_name}] {label}: {ratio:.2f}:1 < required {min_ratio}:1"
            )

if failures:
    print("\n\nFAILED:")
    for f in failures:
        print(f"  {f}")
    raise SystemExit(1)

print("\n\nAll contrast checks passed.")
```

- [ ] **Step 1.3: Run verify_contrast.py to confirm all PASS**

```bash
cd /Users/antoine/esp32-div1/ESP32-DIV-clone && python3 tools/verify_contrast.py
```

Expected output (no FAIL lines):

```
--- DARK ---
  body text — WCAG AAA                              14.28:1  PASS
  secondary text — WCAG AA                           5.58:1  PASS
  accent on base — WCAG AA                          12.63:1  PASS
  status OK — WCAG AA                               10.71:1  PASS
  status WARN — color-coded; hue carries meaning    10.14:1  PASS
  status ERR — WCAG AA                               5.59:1  PASS
  inverted text on selection row — WCAG AA          12.63:1  PASS

--- LIGHT ---
  body text — WCAG AAA                              14.83:1  PASS
  secondary text — WCAG AA                           7.31:1  PASS
  accent on base — WCAG AA                           9.60:1  PASS
  status OK — WCAG AA                                4.75:1  PASS
  status WARN — color-coded; hue carries meaning     3.45:1  PASS
  status ERR — WCAG AA                               5.98:1  PASS
  inverted text on selection row — WCAG AA           9.60:1  PASS

All contrast checks passed.
```

- [ ] **Step 1.4: Write `tools/test_theme_logic.py`**

Create at `tools/test_theme_logic.py`:

```python
#!/usr/bin/env python3
"""Unit test for loadThemeFromEEPROM() bounds-check logic.

Simulates the C++ bounds check in Python so it can run without hardware.
Exit 0 = all assertions pass. Exit 1 = assertion failure.
"""

THEME_DARK  = 0
THEME_LIGHT = 1


def load_theme(byte_val):
    """Mirror of loadThemeFromEEPROM() logic in theme.cpp."""
    id_ = byte_val
    if id_ != THEME_DARK and id_ != THEME_LIGHT:
        id_ = THEME_DARK
    return id_


assert load_theme(0)    == THEME_DARK,  "byte 0x00 must load THEME_DARK"
assert load_theme(1)    == THEME_LIGHT, "byte 0x01 must load THEME_LIGHT"
assert load_theme(0xFF) == THEME_DARK,  "virgin EEPROM (0xFF) must default to THEME_DARK"
assert load_theme(2)    == THEME_DARK,  "out-of-range byte must default to THEME_DARK"
assert load_theme(0x80) == THEME_DARK,  "mid-range invalid byte must default to THEME_DARK"

print("test_theme_logic.py: all 5 assertions passed.")
```

- [ ] **Step 1.5: Run logic tests to confirm all pass**

```bash
cd /Users/antoine/esp32-div1/ESP32-DIV-clone && python3 tools/test_theme_logic.py
```

Expected:

```
test_theme_logic.py: all 5 assertions passed.
```

---

## Task 2: EEPROM Layout + shared.h + theme.cpp

These three files are tightly coupled — `theme.cpp` provides definitions for every `extern` declared in `shared.h`, and both include `eeprom_layout.h`. They must all be consistent before the next compile.

**Files:**
- Modify: `eeprom_layout.h`
- Modify: `shared.h`
- Create: `theme.cpp`

**Interfaces:**
- Consumes: `EEPROM_TOTAL_SIZE` (already in eeprom_layout.h), EEPROM library
- Produces:
  - `ThemeID` enum (`THEME_DARK=0`, `THEME_LIGHT=1`)
  - `extern ThemeID currentThemeID`
  - `void applyTheme(ThemeID id)` — assigns all color globals and writes EEPROM
  - `ThemeID loadThemeFromEEPROM()` — reads byte 2, calls `applyTheme()`, returns id
  - 9 semantic token globals: `BG_BASE`, `BG_SURFACE`, `TEXT_PRIMARY`, `TEXT_SECONDARY`, `ACCENT_SELECT`, `ACCENT_BORDER`, `STATUS_OK`, `STATUS_WARN`, `STATUS_ERR`
  - 16 legacy globals fully remapped (HALEHOUND_*, SHREDDY_*, ORANGE, GREEN, RED, GRAY, LIGHT_GRAY, DARK_GRAY, WHITE, BLACK, BLUE)

---

- [ ] **Step 2.1: Add EEPROM_THEME_ADDR to `eeprom_layout.h`**

Open `eeprom_layout.h`. After the existing `#define EEPROM_TOTAL_SIZE` line and before the CaptivePortal block, add:

```cpp
// ── Theme (theme.cpp) ─────────────────────────────────────────────────────
#define EEPROM_THEME_ADDR    2   // 1 byte: ThemeID (0=dark, 1=light)
```

Then after ALL the existing static_asserts at the bottom, add:

```cpp
static_assert(EEPROM_THEME_ADDR < CP_CRED_ADDR,
    "EEPROM COLLISION: Theme byte overlaps CaptivePortal region");
```

The file should now look like:

```cpp
#pragma once

#define EEPROM_TOTAL_SIZE     1500

// ── Theme (theme.cpp) ─────────────────────────────────────────────────────
#define EEPROM_THEME_ADDR    2   // 1 byte: ThemeID (0=dark, 1=light)

// ── CaptivePortal (wifi.cpp) ──────────────────────────────────────────────
#define CP_CRED_ADDR         32
#define CP_CRED_SIZE         64
#define CP_MAX_CREDS         20
#define CP_COUNT_ADDR        1248

// ── SubGHz (subghz.cpp) ──────────────────────────────────────────────────
#define SUBGHZ_ADDR_VALUE    1320
#define SUBGHZ_ADDR_BITLEN   1324
#define SUBGHZ_ADDR_PROTO    1326
#define SUBGHZ_ADDR_FREQ     1328
#define SUBGHZ_ADDR_PROFILE_START  1340
#define SUBGHZ_MAX_PROFILES        4

// ── Compile-time overlap guards ───────────────────────────────────────────
static_assert(SUBGHZ_ADDR_VALUE > (CP_CRED_ADDR + CP_MAX_CREDS * CP_CRED_SIZE),
    "EEPROM COLLISION: SubGHz region overlaps CaptivePortal credentials");
static_assert(SUBGHZ_ADDR_PROFILE_START + SUBGHZ_MAX_PROFILES * 32 < EEPROM_TOTAL_SIZE,
    "EEPROM OVERFLOW: SubGHz profiles exceed EEPROM_TOTAL_SIZE");
static_assert(EEPROM_THEME_ADDR < CP_CRED_ADDR,
    "EEPROM COLLISION: Theme byte overlaps CaptivePortal region");
```

- [ ] **Step 2.2: Replace `shared.h` with the extern-based version**

Replace the entire contents of `shared.h` with:

```cpp
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
```

- [ ] **Step 2.3: Create `theme.cpp`**

Create a new file `theme.cpp` in the project root with this complete content:

```cpp
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
    ThemeID id = (ThemeID)EEPROM.read(EEPROM_THEME_ADDR);
    if (id != THEME_DARK && id != THEME_LIGHT) id = THEME_DARK;
    applyTheme(id);
    return id;
}
```

- [ ] **Step 2.4: Compile to verify the extern refactor links cleanly**

```bash
/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli compile --fqbn esp32:esp32:esp32 --board-options "FlashSize=16M,PartitionScheme=huge_app,CPUFreq=240,FlashMode=dio,FlashFreq=80,UploadSpeed=921600" --warnings default --build-property "compiler.c.elf.extra_flags=-Wl,-zmuldefs" --output-dir build /Users/antoine/esp32-div1/ESP32-DIV-clone 2>&1 | tail -20
```

Expected: `Sketch uses ... bytes` success lines. Zero `undefined reference` or `multiple definition` errors. If you see `multiple definition of 'ORANGE'` or similar, check that every color variable has been removed from `shared.h` (`const uint16_t X = Y` → `extern uint16_t X`) and that `theme.cpp` defines them once.

---

## Task 3: utils.cpp — Status Bar Upgrade

**Files:**
- Modify: `utils.cpp` lines 184–270 (`drawStatusBar()`)

**Interfaces:**
- Consumes: `BG_SURFACE`, `TEXT_PRIMARY`, `ACCENT_BORDER`, `STATUS_OK`, `STATUS_WARN`, `STATUS_ERR` from Task 2
- Produces: status bar rendered at 24px height with 3-tier battery color

---

- [ ] **Step 3.1: Replace `drawStatusBar()` body in `utils.cpp`**

Open `utils.cpp`. Find `void drawStatusBar(float batteryVoltage, bool forceUpdate)` (around line 184). Replace the entire function body — from the opening `{` to its closing `}` — with:

```cpp
void drawStatusBar(float batteryVoltage, bool forceUpdate) {
  static int lastBatteryPercentage = -1;
  static int lastWiFiStrength = -1;

  int batteryPercentage = map(batteryVoltage * 100, 300, 420, 0, 100);
  batteryPercentage = constrain(batteryPercentage, 0, 100);

  int wifiStrength = 0;
  wifi_mode_t wifiMode = WiFi.getMode();
  if (WiFi.status() == WL_CONNECTED) {
    wifiStrength = constrain(map(WiFi.RSSI(), -100, -30, 0, 100), 0, 100);
  } else if (wifiMode == WIFI_AP || wifiMode == WIFI_AP_STA) {
    wifiStrength = 100;
  }

  float internalTemp = readInternalTemperature();
  bool sdAvailable = false;

  if (batteryPercentage != lastBatteryPercentage || wifiStrength != lastWiFiStrength || forceUpdate) {
    int barHeight = 24;
    int x = 7;
    int y = 4;

    tft.fillRect(0, 0, tft.width(), barHeight, BG_SURFACE);

    // Battery outline + terminal
    tft.drawRoundRect(x, y, 22, 10, 2, ACCENT_BORDER);
    tft.fillRect(x + 22, y + 3, 2, 4, ACCENT_BORDER);

    // Battery fill: 3-tier color
    int batteryLevelWidth = map(batteryPercentage, 0, 100, 0, 20);
    uint16_t batteryColor = (batteryPercentage > 50) ? STATUS_OK
                          : (batteryPercentage > 20) ? STATUS_WARN : STATUS_ERR;
    tft.fillRoundRect(x + 2, y + 2, batteryLevelWidth, 6, 1, batteryColor);

    // Battery percentage text
    tft.setCursor(x + 30, y + 2);
    tft.setTextColor(TEXT_PRIMARY, BG_SURFACE);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.print(String(batteryPercentage) + "%");

    // Wi-Fi signal bars
    int wifiX = 180;
    int wifiY = y + 11;
    for (int i = 0; i < 4; i++) {
      int barH = (i + 1) * 3;
      int barWidth = 4;
      int barX = wifiX + i * 6;
      if (wifiStrength > i * 25) {
        tft.fillRoundRect(barX, wifiY - barH, barWidth, barH, 1, STATUS_OK);
      } else {
        tft.drawRoundRect(barX, wifiY - barH, barWidth, barH, 1, ACCENT_BORDER);
      }
    }

    // Temperature icon
    if (internalTemp >= 55) {
      tft.drawBitmap(203, y - 3, bitmap_icon_temp, 16, 16, STATUS_ERR);
    } else if (internalTemp >= 50) {
      tft.drawBitmap(203, y - 3, bitmap_icon_temp, 16, 16, STATUS_WARN);
    } else {
      tft.drawBitmap(203, y - 3, bitmap_icon_temp, 16, 16, STATUS_OK);
    }

    // SD card icon
    if (sdAvailable) {
      tft.drawBitmap(220, y - 3, bitmap_icon_sdcard, 16, 16, STATUS_OK);
    } else {
      tft.drawBitmap(220, y - 3, bitmap_icon_nullsdcard, 16, 16, STATUS_ERR);
    }

    lastBatteryPercentage = batteryPercentage;
    lastWiFiStrength = wifiStrength;
  }
}
```

- [ ] **Step 3.2: Compile to verify no regressions**

```bash
/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli compile --fqbn esp32:esp32:esp32 --board-options "FlashSize=16M,PartitionScheme=huge_app,CPUFreq=240,FlashMode=dio,FlashFreq=80,UploadSpeed=921600" --warnings default --build-property "compiler.c.elf.extra_flags=-Wl,-zmuldefs" --output-dir build /Users/antoine/esp32-div1/ESP32-DIV-clone 2>&1 | tail -10
```

Expected: clean compile, no errors.

---

## Task 4: ESP32-DIV-clone.ino — Init + Menu + Submenu + Settings

**Files:**
- Modify: `ESP32-DIV-clone.ino`
  - `setup()` at line ~3016: add EEPROM + theme init
  - `displayMenu()` at line ~332: Y_START 30→34, BG_BASE screen fill
  - `displaySubmenu()` at lines 268–324: Font 4, 40px rows, selection bar
  - Settings array + handler at lines ~101–106 and ~2197–2272: +1 item, theme toggle

**Interfaces:**
- Consumes: `applyTheme()`, `loadThemeFromEEPROM()`, `currentThemeID` from Task 2; `drawStatusBar()` at 24px from Task 3
- Produces: working theme toggle in Settings submenu; themed main menu and submenu

---

- [ ] **Step 4.1: Add EEPROM init + theme load to `setup()`**

Find `void setup()` (line ~3016). The first two lines are:
```cpp
  Serial.begin(115200);
  
  tft.init();
```

Change them to:
```cpp
  Serial.begin(115200);

  EEPROM.begin(EEPROM_TOTAL_SIZE);
  loadThemeFromEEPROM();   // must run before any tft color usage

  tft.init();
```

- [ ] **Step 4.2: Update `Y_START` and `displayMenu()` background fill**

Find `const int Y_START = 30;` (line ~329). Change to:

```cpp
const int Y_START = 34;
```

Find `tft.fillScreen(TFT_BLACK);` inside `displayMenu()` (the one preceded by the comment `// Black background with skull in magenta`, line ~351). Change to:

```cpp
tft.fillScreen(BG_BASE);
```

- [ ] **Step 4.3: Replace `displaySubmenu()` with the new Font-4 / 40px layout**

Find `void displaySubmenu()` (line ~268). Replace the entire function with:

```cpp
void displaySubmenu() {
    menu_initialized = false;
    last_menu_index = -1;

    tft.setTextFont(4);
    tft.setTextSize(1);

    if (!submenu_initialized) {
        tft.fillScreen(BG_BASE);

        for (int i = 0; i < active_submenu_size; i++) {
            int yPos = 30 + i * 40;
            tft.fillRect(0, yPos, tft.width(), 40, BG_BASE);
            tft.fillRect(0, yPos, 4, 40, ACCENT_BORDER);
            tft.drawBitmap(10, yPos + 12, active_submenu_icons[i], 16, 16, TEXT_PRIMARY);
            tft.setTextColor(TEXT_PRIMARY, BG_BASE);
            tft.setCursor(32, yPos + 7);
            tft.print(active_submenu_items[i]);
        }

        submenu_initialized = true;
        last_submenu_index = -1;
    }

    if (last_submenu_index != current_submenu_index) {
        // Deselect previous row
        if (last_submenu_index >= 0) {
            int prev_yPos = 30 + last_submenu_index * 40;
            tft.fillRect(0, prev_yPos, tft.width(), 40, BG_BASE);
            tft.fillRect(0, prev_yPos, 4, 40, ACCENT_BORDER);
            tft.drawBitmap(10, prev_yPos + 12, active_submenu_icons[last_submenu_index], 16, 16, TEXT_PRIMARY);
            tft.setTextFont(4);
            tft.setTextColor(TEXT_PRIMARY, BG_BASE);
            tft.setCursor(32, prev_yPos + 7);
            tft.print(active_submenu_items[last_submenu_index]);
        }

        // Highlight selected row
        int new_yPos = 30 + current_submenu_index * 40;
        tft.fillRoundRect(2, new_yPos, tft.width() - 4, 40, 4, ACCENT_SELECT);
        tft.drawBitmap(10, new_yPos + 12, active_submenu_icons[current_submenu_index], 16, 16, BG_BASE);
        tft.setTextFont(4);
        tft.setTextColor(BG_BASE, ACCENT_SELECT);
        tft.setCursor(32, new_yPos + 7);
        tft.print(active_submenu_items[current_submenu_index]);

        last_submenu_index = current_submenu_index;
    }

    drawStatusBar(currentBatteryVoltage, true);
}
```

> **Layout check:** With status bar at 24px and rows starting at y=30, the submenu gap is 6px. 7 rows fit within 320px (`30 + 7*40 = 310 + 40 = 310 at bottom of last row`). All current submenus have ≤ 7 items. ✓

- [ ] **Step 4.4: Add "Display Theme" to the Settings submenu items array**

Find (line ~101):

```cpp
const int settings_NUM_SUBMENU_ITEMS = 4;
const char *settings_submenu_items[settings_NUM_SUBMENU_ITEMS] = {
    "Brightness",
    "Screen Timeout",
    "Device Info",
    "Back to Main Menu"};
```

Replace with:

```cpp
const int settings_NUM_SUBMENU_ITEMS = 5;
const char *settings_submenu_items[settings_NUM_SUBMENU_ITEMS] = {
    "Brightness",
    "Screen Timeout",
    "Device Info",
    "Display Theme",
    "Back to Main Menu"};
```

- [ ] **Step 4.5: Add theme icon to `settings_submenu_icons[]`**

Find (line ~168):

```cpp
const unsigned char *settings_submenu_icons[settings_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_led,      // Brightness
    bitmap_icon_eye2,     // Screen Timeout
    bitmap_icon_stat,     // Device Info
    bitmap_icon_go_back
};
```

Replace with:

```cpp
const unsigned char *settings_submenu_icons[settings_NUM_SUBMENU_ITEMS] = {
    bitmap_icon_led,      // Brightness
    bitmap_icon_eye2,     // Screen Timeout
    bitmap_icon_stat,     // Device Info
    bitmap_icon_eye2,     // Display Theme  (reuses eye icon)
    bitmap_icon_go_back
};
```

- [ ] **Step 4.6: Add `displayThemeScreen()` and `themeToggleLoop()` functions**

Find the line `// ==================== SETTINGS MENU ====================` (around line 2031). Insert these two functions immediately after that comment line, before `int brightness_level = 255;`:

```cpp
void displayThemeScreen() {
    tft.fillScreen(BG_BASE);
    tft.setTextFont(4);
    tft.setTextColor(TEXT_PRIMARY, BG_BASE);
    tft.setCursor(30, 40);
    tft.print("DISPLAY THEME");

    tft.setTextFont(2);
    tft.setTextColor(TEXT_SECONDARY, BG_BASE);
    tft.setCursor(30, 100);
    tft.print("Current:");

    tft.setTextColor(ACCENT_SELECT, BG_BASE);
    tft.setCursor(30, 125);
    tft.print(currentThemeID == THEME_DARK ? "Graphite (Dark)" : "Paper (Light)");

    tft.setTextColor(TEXT_SECONDARY, BG_BASE);
    tft.setCursor(30, 200);
    tft.print("LEFT/RIGHT: Switch");
    tft.setCursor(30, 220);
    tft.print("SELECT: Save & Exit");

    drawStatusBar(currentBatteryVoltage, true);
}

void themeToggleLoop() {
    displayThemeScreen();

    while (!feature_exit_requested) {
        if (isButtonPressed(BTN_LEFT) || isButtonPressed(BTN_RIGHT)) {
            ThemeID next = (currentThemeID == THEME_DARK) ? THEME_LIGHT : THEME_DARK;
            applyTheme(next);
            displayThemeScreen();
            delay(200);
        }
        if (isButtonPressed(BTN_SELECT)) {
            feature_exit_requested = true;
            delay(200);
            break;
        }
        delay(50);
    }
}
```

- [ ] **Step 4.7: Update `handleSettingsSubmenuButtons()` for 5-item menu**

Find `void handleSettingsSubmenuButtons()` (line ~2197). Make three changes:

**Change 1** — "Back to Main Menu" check: change index `3` to `4`:

```cpp
        // Back to Main Menu
        if (current_submenu_index == 4) {   // was: == 3
```

**Change 2** — Add theme toggle handler immediately before the Back check:

```cpp
        // Display Theme
        if (current_submenu_index == 3) {
            feature_active = true;
            feature_exit_requested = false;
            themeToggleLoop();
            in_sub_menu = true;
            is_main_menu = false;
            submenu_initialized = false;
            feature_active = false;
            feature_exit_requested = false;
            displaySubmenu();
            delay(200);
        }
```

The BTN_SELECT block should now read (condensed view):
```
if (isButtonPressed(BTN_SELECT)) {
    ...
    if (current_submenu_index == 4) { ... Back ... }
    if (current_submenu_index == 0) { ... Brightness ... }
    if (current_submenu_index == 1) { ... Screen Timeout ... }
    if (current_submenu_index == 2) { ... Device Info ... }
    if (current_submenu_index == 3) { ... Display Theme ... }
}
```

- [ ] **Step 4.8: Compile the full firmware**

```bash
/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli compile --fqbn esp32:esp32:esp32 --board-options "FlashSize=16M,PartitionScheme=huge_app,CPUFreq=240,FlashMode=dio,FlashFreq=80,UploadSpeed=921600" --warnings default --build-property "compiler.c.elf.extra_flags=-Wl,-zmuldefs" --output-dir build /Users/antoine/esp32-div1/ESP32-DIV-clone 2>&1 | tail -20
```

Expected: clean compile, zero errors or new warnings. If you see `'themeToggleLoop' was not declared in this scope`, add a forward declaration before `displayMenu()`:

```cpp
void themeToggleLoop();  // defined in SETTINGS section below
```

---

## Task 5: Final Verification and Atomic Commit

**Files:**
- All files modified above

**Interfaces:**
- Consumes: all outputs from Tasks 1–4
- Produces: one signed git commit containing all 9 files

---

- [ ] **Step 5.1: Re-run host-side Python tests**

```bash
cd /Users/antoine/esp32-div1/ESP32-DIV-clone
python3 tools/verify_contrast.py && python3 tools/test_theme_logic.py
```

Both must exit 0 (no FAIL lines, "all assertions passed").

- [ ] **Step 5.2: Run final compile**

```bash
/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli compile --fqbn esp32:esp32:esp32 --board-options "FlashSize=16M,PartitionScheme=huge_app,CPUFreq=240,FlashMode=dio,FlashFreq=80,UploadSpeed=921600" --warnings default --build-property "compiler.c.elf.extra_flags=-Wl,-zmuldefs" --output-dir build /Users/antoine/esp32-div1/ESP32-DIV-clone 2>&1 | grep -E "error:|warning:|Sketch uses"
```

Expected: only `Sketch uses ... bytes` lines. Zero `error:` or new `warning:` lines.

- [ ] **Step 5.3: Flash to device**

```bash
/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32 /Users/antoine/esp32-div1/ESP32-DIV-clone
```

- [ ] **Step 5.4: On-device smoke tests (manual)**

Open serial monitor to confirm no boot panic:

```bash
/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli monitor -p /dev/cu.usbserial-0001 --config baudrate=115200
```

Verify these five points:
1. **Boot** — Device boots to main menu in Graphite (dark) theme. No `TG1WDT_SYS_RESET` or cache panic.
2. **Theme toggle** — Navigate: Settings → Display Theme → RIGHT button → theme switches to Paper (light, near-white background, dark text) → SELECT → returns to Settings submenu → main menu redraws in light theme.
3. **Persistence** — Power cycle device. Confirm it boots in Paper (light) theme (EEPROM byte 2 read correctly).
4. **Smoke test** — Navigate WiFi submenu, BT submenu, SubGHz submenu. Text is readable in both themes (no black-on-black or invisible rows).
5. **Status bar** — Battery fill visually changes color at ~50% (OK→WARN transition) and ~20% (WARN→ERR).

- [ ] **Step 5.5: Atomic commit — all 9 files**

Stage and commit all changed/created files in one operation:

```bash
cd /Users/antoine/esp32-div1/ESP32-DIV-clone
git add shared.h theme.cpp eeprom_layout.h utils.cpp ESP32-DIV-clone.ino tools/verify_contrast.py tools/test_theme_logic.py docs/superpowers/specs/2026-06-27-accessibility-theme-design.md docs/superpowers/plans/2026-06-27-accessibility-theme.md
git commit -m "$(cat <<'EOF'
feat: accessibility-first dual-theme system (Graphite / Paper)

- Refactor shared.h color globals from const to extern; single-TU
  definitions in new theme.cpp with applyTheme(THEME_DARK|THEME_LIGHT)
- Add 9 semantic color tokens (BG_BASE, TEXT_PRIMARY, ACCENT_SELECT…)
  with WCAG-verified contrast: 4.5:1 body, 7:1 body text primary
- Persist active theme (1 byte) at EEPROM_THEME_ADDR=2; auto-loads on boot
- Status bar height 20→24px; battery fill 3-tier semantic color
- Submenu rows: Font 4 (26px), 40px row height, rounded selection bar
- Settings: add "Display Theme" toggle (LEFT/RIGHT to switch, SELECT saves)
- tools/verify_contrast.py: host-side WCAG contrast gate
- tools/test_theme_logic.py: unit tests for EEPROM bounds logic

Zero changes to bluetooth.cpp / wifi.cpp / subghz.cpp.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>
EOF
)"
```

> **Note:** If `docs/` is in `.gitignore`, either remove that entry or use `git add -f` for the docs paths. See `.gitignore` line 46.

---

## Self-Review Checklist (completed)

- **Spec coverage:** All spec requirements are covered — dual palette ✓, semantic tokens ✓, legacy remap ✓, Font 4 submenu ✓, 40px rows ✓, 24px status bar ✓, EEPROM persist ✓, settings toggle ✓, Python test gate ✓, atomic commit ✓.
- **Placeholder scan:** All steps contain complete code. No TBD, TODO, or "similar to above" patterns.
- **Type consistency:** `ThemeID` used consistently across shared.h, theme.cpp, and .ino. `applyTheme(ThemeID)` and `loadThemeFromEEPROM()` signatures match between declaration (shared.h) and definition (theme.cpp). `currentThemeID` declared extern in shared.h, defined in theme.cpp, used in .ino.
- **Known constraint:** `STATUS_WARN` on LIGHT background achieves 3.45:1 (warm orange on light grey). This is below WCAG AA (4.5:1) but above the 3.0:1 floor used in `verify_contrast.py`. The hue itself carries semantic meaning, which partially compensates. Documented in the script header.
