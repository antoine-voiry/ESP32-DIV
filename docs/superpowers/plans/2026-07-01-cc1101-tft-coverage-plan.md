# CC1101 Detection, TFT Freeze, Coverage & Left Joystick — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix BUG-006 (CC1101 silent fail), BUG-005 (2–5s TFT freeze on menu return), BUG-001 (left joystick unresponsive), and close TECH-001 unit-test coverage gaps — with every new test targeting a named failure mode.

**Architecture:** Logic-first — extract `computeCC1101Status()` to `lib/logic/` and cover it with adversarial tests before wiring hardware. TFT fix pre-packs the 1-bit XBM as an RGB565 array and replaces the pixel-by-pixel `drawBitmap` call with a single `pushImage()` burst. Left joystick gets a touch-zone fallback that fires when PCF8574 P4 stays stuck-released.

**Tech Stack:** PlatformIO 6.x, `[env:native_tests]` (Unity on native platform), TFT_eSPI 2.5.43 (`pushImage()`), XPT2046 touchscreen (`ts` extern), PCF8574 2.3.7, SmartRC-CC1101 3.0.2.

## Global Constraints

- Buttons read exclusively via `pcf.digitalRead()` — never `pinMode()`/`digitalRead()` on channels 6/3/4/5/7
- No native GPIO 4/6/7 access (flash pins / UART0 RX)
- No ESP32-S3 silicon code
- `-Wl,-zmuldefs` linker flag must remain in `platformio.ini`
- `arduinoFFT` pinned to 1.6.2 — do not upgrade
- Any function with a non-trivial branch goes in `lib/logic/`, not `src/`
- Branch coverage ≥ 90% for every `lib/logic/` unit — every test targets a named failure mode
- Run `platformio test -e native_tests` to verify after each task; run `platformio run` to verify hardware build

---

## File Map

| File | Action | Purpose |
|---|---|---|
| `lib/logic/subghz_logic.h` | CREATE | `computeCC1101Status(bool) → int` declaration |
| `lib/logic/subghz_logic.cpp` | CREATE | Implementation |
| `test/test_subghz/test_main.cpp` | CREATE | 2 adversarial Unity tests |
| `include/hal/ISubGhzRadio.h` | MODIFY | Add `virtual bool isPresent() = 0` |
| `src/hal/CC1101Radio.h` | MODIFY | Add `_present` member, updated `init()`, `isPresent()` |
| `lib/mocks/MockSubGhzRadio.h` | MODIFY | Add `bool present = true` + `isPresent()` |
| `src/main.cpp` | MODIFY | Setup: Serial warn after init; submenu entry: banner after `displaySubmenu()`; feature guards in `handleSubGHzSubmenuButtons()`; BTN_LEFT replacements |
| `tools/gen_rgb565.py` | CREATE | Converts 1-bit XBM → RGB565 PROGMEM array |
| `include/hal9000_bg_rgb.h` | CREATE (generated) | `hal9000_bg_rgb[]` uint16_t PROGMEM array |
| `include/hal/IDisplay.h` | MODIFY | Add `virtual void pushImage(...)` |
| `src/hal/TftDisplay.h` | MODIFY | Delegate `pushImage()` to `_tft.pushImage()` |
| `lib/mocks/MockDisplay.h` | MODIFY | No-op `pushImage()` stub |
| `test/test_hal9000_assets/test_main.cpp` | MODIFY | RGB565 array size regression guard |
| `test/test_theme/test_main.cpp` | MODIFY | 4 `validateThemeRaw` adversarial tests |
| `test/test_wifi_beacon/test_main.cpp` | MODIFY | 2 `buildBeaconTlv` ssidLen=0 tests |
| `test/test_wifi_scan/test_main.cpp` | MODIFY | 1 `compareApByRssi` reversed-pair test |
| `test/test_proximity/test_main.cpp` | MODIFY | 4 `lookupOui` adversarial tests |
| `src/utils.cpp` | MODIFY | Add `isTouchLeft()` |
| `include/utils.h` (or `include/shared.h`) | MODIFY | Declare `isTouchLeft()` |

---

## Task 1: CC1101 Logic Unit + Tests (TDD — tests first)

**Files:**
- Create: `lib/logic/subghz_logic.h`
- Create: `lib/logic/subghz_logic.cpp`
- Create: `test/test_subghz/test_main.cpp`

**Interfaces:**
- Produces: `int computeCC1101Status(bool chipDetected)` — returns 0 (OK) when chip detected, 1 (NOT_DETECTED) otherwise

- [ ] **Step 1: Write the failing tests**

Create `test/test_subghz/test_main.cpp`:

```cpp
#include <unity.h>

extern "C" int computeCC1101Status(bool chipDetected);

void setUp()    {}
void tearDown() {}

// Target failure mode: inverted return values (detected→1, not detected→0)
void test_cc1101_detected_returns_ok() {
    TEST_ASSERT_EQUAL_INT(0, computeCC1101Status(true));
}

void test_cc1101_absent_returns_not_detected() {
    TEST_ASSERT_EQUAL_INT(1, computeCC1101Status(false));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_cc1101_detected_returns_ok);
    RUN_TEST(test_cc1101_absent_returns_not_detected);
    return UNITY_END();
}
```

- [ ] **Step 2: Run to confirm FAIL**

```bash
cd /Users/antoine/esp32-div1/ESP32-DIV-clone
platformio test -e native_tests --filter test_subghz
```

Expected: compilation error — `computeCC1101Status` not declared.

- [ ] **Step 3: Create the header**

Create `lib/logic/subghz_logic.h`:

```cpp
#pragma once
#include <stdbool.h>

// Returns 0=OK when chip responds to register readback, 1=NOT_DETECTED otherwise.
int computeCC1101Status(bool chipDetected);
```

- [ ] **Step 4: Create the implementation**

Create `lib/logic/subghz_logic.cpp`:

```cpp
#include "subghz_logic.h"

int computeCC1101Status(bool chipDetected) {
    return chipDetected ? 0 : 1;
}
```

- [ ] **Step 5: Run to confirm PASS**

```bash
platformio test -e native_tests --filter test_subghz
```

Expected:
```
test/test_subghz/test_main.cpp:11:test_cc1101_detected_returns_ok PASSED
test/test_subghz/test_main.cpp:15:test_cc1101_absent_returns_not_detected PASSED
```

- [ ] **Step 6: Commit**

```bash
git add lib/logic/subghz_logic.h lib/logic/subghz_logic.cpp test/test_subghz/test_main.cpp
git commit -m "feat(logic): add computeCC1101Status — tested before HAL wire-up"
```

---

## Task 2: HAL Layer — isPresent() on ISubGhzRadio, CC1101Radio, MockSubGhzRadio

**Files:**
- Modify: `include/hal/ISubGhzRadio.h`
- Modify: `src/hal/CC1101Radio.h`
- Modify: `lib/mocks/MockSubGhzRadio.h`

**Interfaces:**
- Consumes: nothing from Task 1 at compile time (logic unit is separate)
- Produces: `gSubGhz->isPresent()` returning `bool` — used in Task 3

- [ ] **Step 1: Add `isPresent()` to `ISubGhzRadio`**

Edit `include/hal/ISubGhzRadio.h` — add one line after `checkReceiveFlag`:

```cpp
#pragma once
#include <stdint.h>

struct ISubGhzRadio {
    virtual void    init() = 0;
    virtual void    setFrequency(float mhz) = 0;
    virtual bool    sendData(uint8_t* data, uint8_t len) = 0;
    virtual uint8_t receiveData(uint8_t* data) = 0;
    virtual bool    checkReceiveFlag() = 0;
    virtual bool    isPresent() = 0;
    virtual ~ISubGhzRadio() = default;
};
```

- [ ] **Step 2: Update `CC1101Radio` — store presence result from `getCC1101()`**

Replace the full content of `src/hal/CC1101Radio.h`:

```cpp
// src/hal/CC1101Radio.h
#pragma once
#include <ELECHOUSE_CC1101_ESP32DIV.h>
#include "hal/ISubGhzRadio.h"
#include "subghz_logic.h"

class CC1101Radio : public ISubGhzRadio {
    bool _present = false;
public:
    void init() override {
        ELECHOUSE_cc1101.Init();
        _present = ELECHOUSE_cc1101.getCC1101();
    }
    bool    isPresent() override                             { return _present; }
    void    setFrequency(float mhz) override                 { ELECHOUSE_cc1101.setMHZ(mhz); }
    bool    sendData(uint8_t* data, uint8_t len) override    { ELECHOUSE_cc1101.SendData(data, len); return true; }
    uint8_t receiveData(uint8_t* data) override              { return ELECHOUSE_cc1101.ReceiveData(data); }
    bool    checkReceiveFlag() override                      { return ELECHOUSE_cc1101.CheckReceiveFlag(); }
};
```

- [ ] **Step 3: Update `MockSubGhzRadio` — injectable presence state**

Replace the full content of `lib/mocks/MockSubGhzRadio.h`:

```cpp
// lib/mocks/MockSubGhzRadio.h
#pragma once
#include "hal/ISubGhzRadio.h"

struct MockSubGhzRadio : ISubGhzRadio {
    bool present = true;   // set to false in tests to simulate absent chip
    void    init() override                           {}
    bool    isPresent() override                      { return present; }
    void    setFrequency(float) override              {}
    bool    sendData(uint8_t*, uint8_t) override      { return true; }
    uint8_t receiveData(uint8_t*) override            { return 0; }
    bool    checkReceiveFlag() override               { return false; }
};
```

- [ ] **Step 4: Verify hardware build still compiles**

```bash
platformio run
```

Expected: `SUCCESS` — no new errors. If `CC1101Radio.h` now includes `subghz_logic.h`, confirm `lib/logic/` is on the include path (it is via PlatformIO's `lib_dir` auto-discovery).

- [ ] **Step 5: Commit**

```bash
git add include/hal/ISubGhzRadio.h src/hal/CC1101Radio.h lib/mocks/MockSubGhzRadio.h
git commit -m "feat(hal): add isPresent() to ISubGhzRadio — CC1101 register-readback detection"
```

---

## Task 3: BUG-006 Wire-Up — Serial Warning + SubGHz UI Banner

**Files:**
- Modify: `src/main.cpp` — two locations: setup() Serial warn, handleSubGHzSubmenuButtons() banner + feature guards, handleButtons() initial submenu entry banner

**Interfaces:**
- Consumes: `gSubGhz->isPresent()` (Task 2)

- [ ] **Step 1: Add Serial warning in `setup()` after `gSubGhz` init**

In `src/main.cpp`, find the `setup()` function around line 3282 where `gSubGhz = &halSubGhz` is assigned. The `gSubGhz->init()` call must exist — if it does not yet exist, add it. After the init call, add:

```cpp
    gSubGhz  = &halSubGhz;
    gSubGhz->init();
    if (!gSubGhz->isPresent()) {
        Serial.println("[CC1101] not detected — SubGHz disabled");
    }
```

Check: `gSubGhz->init()` was previously only called lazily or not at all. If absent from setup(), add it immediately after the pointer assignment above.

- [ ] **Step 2: Add helper `drawCC1101AbsentBanner()`**

In `src/main.cpp`, directly above `void handleSubGHzSubmenuButtons()` (line ~1682), add:

```cpp
static void drawCC1101AbsentBanner() {
    tft.fillRect(0, 0, 240, 22, STATUS_ERR);
    tft.setTextColor(TFT_WHITE, STATUS_ERR);
    tft.setTextFont(2);
    tft.drawString("CC1101 NOT DETECTED", 10, 4, 2);
}
```

- [ ] **Step 3: Draw banner after initial submenu entry in `handleButtons()`**

In `handleButtons()`, find the SELECT handler (around line 3196) where `displaySubmenu()` is called:

```cpp
            if (active_submenu_items && active_submenu_size > 0) {
                current_submenu_index = 0;
                in_sub_menu = true;
                submenu_initialized = false;
                displaySubmenu();
            }
```

Add the banner draw immediately after `displaySubmenu()`:

```cpp
            if (active_submenu_items && active_submenu_size > 0) {
                current_submenu_index = 0;
                in_sub_menu = true;
                submenu_initialized = false;
                displaySubmenu();
                if (current_menu_index == 3 && !gSubGhz->isPresent()) {
                    drawCC1101AbsentBanner();
                }
            }
```

- [ ] **Step 4: Draw banner after UP/DOWN redraws in `handleSubGHzSubmenuButtons()`**

In `handleSubGHzSubmenuButtons()`, after each `displaySubmenu()` call (lines ~1689 and ~1700), add the banner draw:

```cpp
    if (isButtonPressed(BTN_UP)) {
        current_submenu_index = (current_submenu_index - 1 + active_submenu_size) % active_submenu_size;
        if (current_submenu_index < 0) {
            current_submenu_index = NUM_SUBMENU_ITEMS - 1;
        }
        last_interaction_time = millis();
        displaySubmenu();
        if (!gSubGhz->isPresent()) drawCC1101AbsentBanner();
        delay(200);
    }

    if (isButtonPressed(BTN_DOWN)) {
        current_submenu_index = (current_submenu_index + 1) % active_submenu_size;
        if (current_submenu_index >= NUM_SUBMENU_ITEMS) {
            current_submenu_index = 0;
        }
        last_interaction_time = millis();
        displaySubmenu();
        if (!gSubGhz->isPresent()) drawCC1101AbsentBanner();
        delay(200);
    }
```

- [ ] **Step 5: Guard feature launches in `handleSubGHzSubmenuButtons()`**

In `handleSubGHzSubmenuButtons()`, inside the SELECT handler (line ~1703), add a guard before any feature is launched (indices 0–3). The back button (index 4) is unguarded. Add this at the top of the SELECT block, before the `if (current_submenu_index == 4)` check:

```cpp
    if (isButtonPressed(BTN_SELECT)) {
        last_interaction_time = millis();
        delay(200);

        if (!gSubGhz->isPresent() && current_submenu_index != 4) {
            drawCC1101AbsentBanner();
            return;
        }

        if (current_submenu_index == 4) {
            // ... existing back logic
```

- [ ] **Step 6: Build and verify**

```bash
platformio run
```

Expected: `SUCCESS`.

- [ ] **Step 7: Commit**

```bash
git add src/main.cpp
git commit -m "fix(BUG-006): CC1101 presence detection — Serial warn + SubGHz UI banner"
```

---

## Task 4: BUG-005 Part A — Generate RGB565 Array

**Files:**
- Create: `tools/gen_rgb565.py`
- Create: `include/hal9000_bg_rgb.h` (run the script)

**Interfaces:**
- Produces: `const uint16_t hal9000_bg_rgb[]` PROGMEM — 211×280 = 59,080 uint16_t words

- [ ] **Step 1: Create the generator script**

Create `tools/gen_rgb565.py`:

```python
#!/usr/bin/env python3
"""Convert hal9000_bg_bitmap (1-bit XBM) to RGB565 PROGMEM array.
Set bits → 0xF800 (red), clear bits → 0x0000 (black).
Run from repo root: python3 tools/gen_rgb565.py
"""
import re, math, sys
from pathlib import Path

SRC  = Path('include/hal9000_bg.h')
DEST = Path('include/hal9000_bg_rgb.h')

src = SRC.read_text()

w = int(re.search(r'HAL9000_BG_WIDTH\s+(\d+)',  src).group(1))
h = int(re.search(r'HAL9000_BG_HEIGHT\s+(\d+)', src).group(1))

raw_bytes = [int(x, 16) for x in re.findall(r'0x[0-9a-fA-F]+', src)]
bytes_per_row = math.ceil(w / 8)

expected = bytes_per_row * h
if len(raw_bytes) < expected:
    sys.exit(f"ERROR: expected {expected} bytes, got {len(raw_bytes)}")

pixels = []
for row in range(h):
    for col in range(w):
        b   = raw_bytes[row * bytes_per_row + col // 8]
        bit = (b >> (col % 8)) & 1          # XBM is LSB-first
        pixels.append(0xF800 if bit else 0x0000)

lines = [
    '#pragma once',
    '#include <stdint.h>',
    f'#define HAL9000_BG_RGB_WIDTH  {w}',
    f'#define HAL9000_BG_RGB_HEIGHT {h}',
    f'const uint16_t hal9000_bg_rgb[{w * h}] PROGMEM = {{',
]
for i in range(0, len(pixels), 16):
    chunk = pixels[i:i+16]
    row_str = ', '.join(f'0x{p:04X}' for p in chunk)
    suffix  = ',' if (i + 16) < len(pixels) else ''
    lines.append(f'    {row_str}{suffix}')
lines.append('};')

DEST.write_text('\n'.join(lines) + '\n')
print(f"Written {DEST}: {w}x{h} = {len(pixels)} pixels, {len(pixels)*2} bytes")
```

- [ ] **Step 2: Run the script**

```bash
cd /Users/antoine/esp32-div1/ESP32-DIV-clone
python3 tools/gen_rgb565.py
```

Expected output: `Written include/hal9000_bg_rgb.h: 211x280 = 59080 pixels, 118160 bytes`

- [ ] **Step 3: Spot-check the generated header**

```bash
head -8 include/hal9000_bg_rgb.h && wc -l include/hal9000_bg_rgb.h
```

Expected: header starts with `#pragma once`, `#include <stdint.h>`, two `#define` lines, then `const uint16_t hal9000_bg_rgb[59080] PROGMEM = {`. Values should be only `0xF800` or `0x0000`.

- [ ] **Step 4: Commit**

```bash
git add tools/gen_rgb565.py include/hal9000_bg_rgb.h
git commit -m "feat(assets): gen_rgb565.py — pre-pack HAL9000 background as RGB565 PROGMEM array"
```

---

## Task 5: BUG-005 Part B — IDisplay::pushImage + swap drawBitmap + regression test

**Files:**
- Modify: `include/hal/IDisplay.h` — add `pushImage()`
- Modify: `src/hal/TftDisplay.h` — delegate `pushImage()`
- Modify: `lib/mocks/MockDisplay.h` — no-op stub
- Modify: `src/main.cpp:371` — swap render call
- Modify: `test/test_hal9000_assets/test_main.cpp` — array size guard

**Interfaces:**
- Consumes: `hal9000_bg_rgb.h` (Task 4)
- Produces: `gDisplay->pushImage(x, y, w, h, data)` callable from `src/`

- [ ] **Step 1: Add `pushImage()` to `IDisplay`**

In `include/hal/IDisplay.h`, add after the existing `drawBitmap` virtual:

```cpp
    virtual void pushImage(int16_t x, int16_t y, int16_t w, int16_t h,
                           const uint16_t* data) = 0;
```

- [ ] **Step 2: Implement in `TftDisplay`**

In `src/hal/TftDisplay.h`, add after the `drawBitmap` line:

```cpp
    void pushImage(int16_t x, int16_t y, int16_t w, int16_t h,
                   const uint16_t* data) override {
        _tft.pushImage(x, y, w, h, data);
    }
```

- [ ] **Step 3: Add no-op stub to `MockDisplay`**

In `lib/mocks/MockDisplay.h`, add after the `drawBitmap` stub:

```cpp
    void pushImage(int16_t, int16_t, int16_t, int16_t,
                   const uint16_t*) override {}
```

- [ ] **Step 4: Swap the render call in `displayMenu()`**

In `src/main.cpp`, add the include at the top (near other `#include` for bitmap headers):

```cpp
#include "hal9000_bg_rgb.h"
```

Then find the slow render at `src/main.cpp:371` (inside `displayMenu()`, inside the `if (!menu_initialized)` block):

```cpp
        // Draw HAL9000 eye in red
        tft.drawBitmap(hal9000X, hal9000Y, hal9000_bg_bitmap, HAL9000_BG_WIDTH, HAL9000_BG_HEIGHT, 0xF800);
```

Replace with:

```cpp
        // Burst-transfer pre-packed RGB565 — replaces slow pixel-by-pixel drawBitmap (BUG-005)
        tft.pushImage(hal9000X, hal9000Y, HAL9000_BG_RGB_WIDTH, HAL9000_BG_RGB_HEIGHT, hal9000_bg_rgb);
```

- [ ] **Step 5: Add RGB565 array size regression guard to `test_hal9000_assets`**

In `test/test_hal9000_assets/test_main.cpp`, add the include and test. Add near the top includes:

```cpp
#include "arduino_progmem_shim.h"
#include "hal9000_bg_rgb.h"
```

Add this test function (and register it in `main()`):

```cpp
void test_hal9000_bg_rgb_pixel_count() {
    // Guards against regenerating the array at wrong dimensions
    TEST_ASSERT_EQUAL(HAL9000_BG_RGB_WIDTH * HAL9000_BG_RGB_HEIGHT,
                      (int)(sizeof(hal9000_bg_rgb) / sizeof(uint16_t)));
}
```

Add `RUN_TEST(test_hal9000_bg_rgb_pixel_count);` to `main()`.

- [ ] **Step 6: Run native tests**

```bash
platformio test -e native_tests --filter test_hal9000_assets
```

Expected: all existing tests PASS + new `test_hal9000_bg_rgb_pixel_count` PASS.

- [ ] **Step 7: Build hardware target**

```bash
platformio run
```

Expected: `SUCCESS`. If PROGMEM causes an issue in the native test build, confirm `arduino_progmem_shim.h` defines `#define PROGMEM` (it does).

- [ ] **Step 8: Commit**

```bash
git add include/hal/IDisplay.h src/hal/TftDisplay.h lib/mocks/MockDisplay.h \
        src/main.cpp test/test_hal9000_assets/test_main.cpp
git commit -m "fix(BUG-005): replace drawBitmap with pushImage for HAL9000 bg — eliminates 2-5s freeze"
```

---

## Task 6: TECH-001 Coverage Gaps — theme, beacon, wifi_scan

**Files:**
- Modify: `test/test_theme/test_main.cpp`
- Modify: `test/test_wifi_beacon/test_main.cpp`
- Modify: `test/test_wifi_scan/test_main.cpp`

**Interfaces:**
- Consumes: `validateThemeRaw(uint8_t) → ThemeID` (in `lib/logic/theme_logic.h`; `THEME_DARK=0`, `THEME_LIGHT=1`)
- Consumes: `buildBeaconTlv(packet, bufLen, ssid, ssidLen, channel) → int`
- Consumes: `compareApByRssi(const void*, const void*) → int`

- [ ] **Step 1: Add `validateThemeRaw` tests to `test/test_theme/test_main.cpp`**

Add these 4 test functions. Each comment names the failure mode it catches:

```cpp
// validateThemeRaw — 4 tests targeting specific implementation mistakes

// Failure mode: swapped case order → raw=0 returns THEME_LIGHT
void test_validate_raw_0_is_dark() {
    TEST_ASSERT_EQUAL_INT(THEME_DARK, (int)validateThemeRaw(0));
}

// Failure mode: swapped case order → raw=1 returns THEME_DARK
void test_validate_raw_1_is_light() {
    TEST_ASSERT_EQUAL_INT(THEME_LIGHT, (int)validateThemeRaw(1));
}

// Failure mode: missing default clause → raw=2 returns garbage or asserts
void test_validate_raw_2_fallback_dark() {
    TEST_ASSERT_EQUAL_INT(THEME_DARK, (int)validateThemeRaw(2));
}

// Failure mode: fallback returning THEME_LIGHT instead of THEME_DARK
void test_validate_raw_255_fallback_dark() {
    TEST_ASSERT_EQUAL_INT(THEME_DARK, (int)validateThemeRaw(255));
}
```

Register all four in `main()` with `RUN_TEST(...)`.

- [ ] **Step 2: Add `buildBeaconTlv` ssidLen=0 tests to `test/test_wifi_beacon/test_main.cpp`**

`buildBeaconTlv` with ssidLen=0: `dsOffset = 38+0+10 = 48`, `totalLen = 48+3 = 51`. `packet[37]` is the SSID length byte.

```cpp
// Failure mode: off-by-one → length byte written as 1 instead of 0
void test_buildBeaconTlv_ssid_zero_len_byte() {
    uint8_t buf[64] = {};
    buildBeaconTlv(buf, sizeof(buf), "", 0, 6);
    TEST_ASSERT_EQUAL_UINT8(0, buf[37]);
}

// Failure mode: size calculation adds 1 for empty SSID → returns 52 instead of 51
void test_buildBeaconTlv_ssid_zero_return_size() {
    uint8_t buf[64] = {};
    int result = buildBeaconTlv(buf, sizeof(buf), "", 0, 6);
    TEST_ASSERT_EQUAL_INT(51, result);
}
```

Register both in `main()`.

- [ ] **Step 3: Add `compareApByRssi` reversed-pair test to `test/test_wifi_scan/test_main.cpp`**

```cpp
// Failure mode: sign inversion → ascending sort instead of descending
// a.rssi=-80 (worse), b.rssi=-60 (better): result must be >0 (b sorts before a)
void test_compare_lower_rssi_sorts_after() {
    WifiApInfo worse = {}; worse.rssi = -80;
    WifiApInfo better = {}; better.rssi = -60;
    int result = compareApByRssi(&worse, &better);
    TEST_ASSERT_GREATER_THAN(0, result);
}
```

Register in `main()`.

- [ ] **Step 4: Run all three suites**

```bash
platformio test -e native_tests --filter "test_theme|test_wifi_beacon|test_wifi_scan"
```

Expected: all tests PASS including the 7 new ones.

- [ ] **Step 5: Commit**

```bash
git add test/test_theme/test_main.cpp test/test_wifi_beacon/test_main.cpp \
        test/test_wifi_scan/test_main.cpp
git commit -m "test(TECH-001): adversarial coverage gaps — validateThemeRaw, buildBeaconTlv ssidLen=0, compareApByRssi reversed"
```

---

## Task 7: TECH-001 Coverage Gaps — proximity lookupOui

**Files:**
- Modify: `test/test_proximity/test_main.cpp`

**Interfaces:**
- Consumes: `lookupOui(mac3, table, entryCount, vendorOut, threatOut) → bool` (from `lib/logic/proximity_logic.h`)
- Consumes: `struct OuiEntry { uint8_t oui[3]; char vendor[16]; bool threat; }` (packed, 20 bytes)

- [ ] **Step 1: Add 4 adversarial `lookupOui` tests**

Add these functions to `test/test_proximity/test_main.cpp`. Include `<string.h>` if not already present.

```cpp
// ── lookupOui adversarial tests ───────────────────────────────────────────────

// Failure mode: vendorOut or *threatOut left uninitialized on miss
void test_lookup_not_found_clears_outputs() {
    OuiEntry table[1] = {{{0xAA, 0xBB, 0xCC}, "Vendor", false}};
    uint8_t mac[3]  = {0x11, 0x22, 0x33};
    char    vendor[16] = "GARBAGE";  // pre-fill to catch uncleared output
    bool    threat = true;            // pre-set to true to catch uncleared flag
    bool found = lookupOui(mac, table, 1, vendor, &threat);
    TEST_ASSERT_FALSE(found);
    TEST_ASSERT_EQUAL_CHAR('\0', vendor[0]);
    TEST_ASSERT_FALSE(threat);
}

// Failure mode: loop bounds < vs <= → entryCount=0 crashes or skips
void test_lookup_empty_table_returns_false() {
    char    vendor[16] = {};
    bool    threat = false;
    uint8_t mac[3] = {0x01, 0x02, 0x03};
    bool found = lookupOui(mac, nullptr, 0, vendor, &threat);
    TEST_ASSERT_FALSE(found);
    TEST_ASSERT_EQUAL_CHAR('\0', vendor[0]);
}

// Failure mode: early-exit after index 0 → never searches index 1+
void test_lookup_found_at_second_entry() {
    OuiEntry table[2] = {
        {{0xAA, 0xBB, 0xCC}, "First",  false},
        {{0x00, 0x1A, 0x2B}, "Second", false},
    };
    uint8_t mac[3] = {0x00, 0x1A, 0x2B};
    char    vendor[16] = {};
    bool    threat = false;
    bool found = lookupOui(mac, table, 2, vendor, &threat);
    TEST_ASSERT_TRUE(found);
    TEST_ASSERT_EQUAL_STRING("Second", vendor);
}

// Failure mode: threat flag never written → always returns false
void test_lookup_threat_flag_set() {
    OuiEntry table[1] = {{{0xDE, 0xAD, 0xBE}, "EvilCorp", true}};
    uint8_t mac[3] = {0xDE, 0xAD, 0xBE};
    char    vendor[16] = {};
    bool    threat = false;
    bool found = lookupOui(mac, table, 1, vendor, &threat);
    TEST_ASSERT_TRUE(found);
    TEST_ASSERT_TRUE(threat);
}
```

Register all four in `main()`.

- [ ] **Step 2: Run**

```bash
platformio test -e native_tests --filter test_proximity
```

Expected: all existing tests + 4 new ones PASS. If any of the 4 new tests **FAIL**, that is a real bug in `lookupOui` — do not suppress it; fix `lib/logic/proximity_logic.h` first, then re-run.

- [ ] **Step 3: Commit**

```bash
git add test/test_proximity/test_main.cpp
git commit -m "test(TECH-001): adversarial lookupOui coverage — not-found, empty table, index-1, threat flag"
```

---

## Task 8: BUG-001 — Left Joystick Touch Fallback

**Files:**
- Modify: `src/utils.cpp` — add `isTouchLeft()`
- Modify: `include/shared.h` (or wherever utils functions are declared) — declare `isTouchLeft()`
- Modify: `src/main.cpp` — replace 5 `isButtonPressed(BTN_LEFT)` calls

**Context:** PCF8574 P4 (BTN_LEFT=4) may be hardware-damaged — `pcf.digitalRead(4)` always returns HIGH (Released). The fix provides a touch-zone fallback: a tap in the leftmost 40px of the screen acts as BTN_LEFT.

- [ ] **Step 1: Verify the existing BTN_LEFT call sites**

```bash
grep -n "isButtonPressed(BTN_LEFT)" /Users/antoine/esp32-div1/ESP32-DIV-clone/src/main.cpp
```

Expected: 5 occurrences at approximately lines 2075, 2125, 2188, 2310, 3159.

- [ ] **Step 2: Check where utils functions are declared**

```bash
grep -rn "readBatteryVoltage\|drawStatusBar" /Users/antoine/esp32-div1/ESP32-DIV-clone/include/ 2>/dev/null | head -5
```

This tells you which header declares `src/utils.cpp` functions. Add `isTouchLeft()` declaration to that same header.

- [ ] **Step 3: Add `isTouchLeft()` to `src/utils.cpp`**

At the bottom of `src/utils.cpp`, before the closing of any namespace or after the last function, add:

```cpp
bool isTouchLeft() {
    if (!ts.touched()) return false;
    TS_Point p = ts.getPoint();
    int x = map(p.x, TS_MINX, TS_MAXX, 0, 239);
    return x < 40;
}
```

`ts`, `TS_MINX`, `TS_MAXX` are from `#include "Touchscreen.h"` — confirm this include exists at the top of `utils.cpp`. If not, add it.

- [ ] **Step 4: Declare `isTouchLeft()` in the appropriate header**

In whichever header was found in Step 2, add:

```cpp
bool isTouchLeft();
```

- [ ] **Step 5: Replace all 5 `isButtonPressed(BTN_LEFT)` calls in `src/main.cpp`**

Replace every occurrence of:
```cpp
isButtonPressed(BTN_LEFT)
```
with:
```cpp
(isButtonPressed(BTN_LEFT) || isTouchLeft())
```

Run `grep -n "isButtonPressed(BTN_LEFT)" src/main.cpp` after to confirm zero remaining occurrences.

- [ ] **Step 6: Build**

```bash
platformio run
```

Expected: `SUCCESS`.

- [ ] **Step 7: Commit**

```bash
git add src/utils.cpp include/shared.h src/main.cpp
git commit -m "fix(BUG-001): left joystick touch fallback — x<40 tap zone when PCF8574 P4 stuck-released"
```

---

## Final Verification

- [ ] **Run full native test suite**

```bash
platformio test -e native_tests
```

Expected: all suites PASS with no skips. New test count vs baseline:
- `test_subghz`: +2
- `test_theme`: +4
- `test_wifi_beacon`: +2
- `test_wifi_scan`: +1
- `test_proximity`: +4
- `test_hal9000_assets`: +1
- **Total new: 14 tests**

- [ ] **Run hardware build**

```bash
platformio run
```

Expected: `SUCCESS` with no new warnings.

- [ ] **Update bug list**

In `docs/bugs/bug-list.md`:
- BUG-006: change `**Status:** Open` → `**Status:** Fixed`, add `**Fixed:** 2026-07-01`
- BUG-005: same
- BUG-001: same (workaround via touch fallback; note hardware diagnosis still pending)
- TECH-001: update to reflect coverage closed for subghz, theme, beacon, scan, proximity
