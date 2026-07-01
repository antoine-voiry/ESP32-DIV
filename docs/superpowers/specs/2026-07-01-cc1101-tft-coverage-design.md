# Design: CC1101 Detection, TFT Bitmap Freeze, Unit Test Coverage

**Date:** 2026-07-01  
**Bugs addressed:** BUG-006, BUG-005, TECH-001  
**Approach:** Option 2 — logic-first extraction, then hardware wire-up

---

## 1. BUG-006 — CC1101 Presence Detection

### Problem

`CC1101Radio::init()` calls `ELECHOUSE_cc1101.Init()` with no return value check and never calls `getCC1101()` (register-readback presence verification). If the CC1101 is absent, powered off, or blocked on SPI at init time, the failure is silent — all subsequent SubGHz operations produce garbage. `main.cpp` calls `gSubGhz->init()` unconditionally with no status check.

### Logic layer (`lib/logic/subghz_logic.h/.cpp`)

New file pair. One pure function:

```cpp
// Returns 0=OK, 1=NOT_DETECTED
int computeCC1101Status(bool chipDetected);
```

Keeps the branch in `lib/logic/` per CLAUDE.md architecture mandate. Covered by 2 Unity tests in `test/test_subghz/test_main.cpp`.

### HAL layer

`ISubGhzRadio` (`include/hal/ISubGhzRadio.h`) gains one new pure virtual:

```cpp
virtual bool isPresent() = 0;
```

`CC1101Radio` (`src/hal/CC1101Radio.h/.cpp`):
- Adds `bool _present = false;` member
- `init()` calls `ELECHOUSE_cc1101.Init()` then `getCC1101()`, stores result in `_present`
- `isPresent()` returns `_present`

`MockSubGhzRadio` (`lib/mocks/MockSubGhzRadio.h`):
- Adds `bool present = true;`
- `isPresent() override { return present; }`

### Wire-up (`src/main.cpp` setup)

After `gSubGhz->init()`:

```cpp
if (!gSubGhz->isPresent()) {
    Serial.println("[CC1101] not detected — SubGHz disabled");
}
```

### UI (`src/subghz.cpp` submenu entry)

At the top of the SubGHz submenu render function, before drawing menu items:

```cpp
if (!gSubGhz->isPresent()) {
    // Red banner across submenu header
    tft.fillRect(0, 0, 240, 20, STATUS_ERR);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("CC1101 NOT DETECTED", 10, 4, 2);
}
```

All SubGHz feature setup functions check `gSubGhz->isPresent()` at entry and return early (showing the same banner) if false. Menu items remain navigable.

---

## 2. BUG-005 — TFT Background Bitmap Freeze

### Problem

`tft.drawBitmap()` renders the 211×280 HAL9000 XBM by calling `drawPixel()` once per set bit — up to 59,080 individual SPI writes. At 40 MHz SPI this takes 2–5 seconds. `displaySubmenu()` resets `menu_initialized = false` on every entry, so every return to the main menu triggers a full redraw. This is also the root cause of BUG-004 (buttons unresponsive after About → back).

### RGB565 pre-packed array (`include/hal9000_bg_rgb.h`)

A Python script `tools/gen_rgb565.py` reads `hal9000_bg_bitmap` (1-bit XBM) and expands each bit to a 16-bit RGB565 word:
- Set bit → `0xF800` (red)
- Clear bit → `0x0000` (black)

Output: `const uint16_t hal9000_bg_rgb[] PROGMEM` — 211×280 = 59,080 words (118,160 bytes).

The script is committed to `tools/`. The generated header is also committed. Re-run only if the source bitmap changes.

PROGMEM cost: 118 KB. Acceptable — partition is `huge_app.csv` on 16 MB flash.

### Render call (`src/main.cpp:371`)

```cpp
// Remove:
tft.drawBitmap(hal9000X, hal9000Y, hal9000_bg_bitmap, HAL9000_BG_WIDTH, HAL9000_BG_HEIGHT, 0xF800);

// Replace with:
tft.pushImage(hal9000X, hal9000Y, HAL9000_BG_WIDTH, HAL9000_BG_HEIGHT, hal9000_bg_rgb);
```

`IDisplay` (`include/hal/IDisplay.h`) gains:

```cpp
virtual void pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) = 0;
```

`TftDisplay` delegates to `_tft.pushImage(x, y, w, h, data)`.  
`MockDisplay` adds a no-op stub.

`drawBitmap()` remains on `IDisplay` — the 16×16 icon draws are fast and unchanged.

### `menu_initialized` logic

Unchanged. Submenus overwrite the full screen, so the background genuinely must be redrawn on return. The fix is that the redraw now takes ~20ms instead of 2–5s.

### Regression guard

Add to `test/test_hal9000_assets/test_main.cpp`:

```cpp
void test_hal9000_bg_rgb_size() {
    TEST_ASSERT_EQUAL(HAL9000_BG_WIDTH * HAL9000_BG_HEIGHT,
                      (int)(sizeof(hal9000_bg_rgb) / sizeof(uint16_t)));
}
```

---

## 3. TECH-001 — Unit Test Coverage

### Constraint

Every test must target a named, plausible implementation mistake. Coverage for its own sake is rejected.

### New test suite: `test/test_subghz/test_main.cpp`

| Test | Failure mode caught |
|---|---|
| `chipDetected=true → 0` | inverted return values |
| `chipDetected=false → 1` | inverted return values |

### Gap fills in existing suites

#### `test_theme` — `validateThemeRaw` (0 tests today)

| Test | Failure mode caught |
|---|---|
| `raw=0 → THEME_DARK` | swapped case order in switch |
| `raw=1 → THEME_LIGHT` | swapped case order |
| `raw=2 → THEME_DARK` (fallback) | missing `default` clause |
| `raw=255 → THEME_DARK` (fallback) | fallback returning wrong theme |

#### `test_wifi_beacon` — `buildBeaconTlv` edge cases

| Test | Failure mode caught |
|---|---|
| `ssidLen=0`: TLV length byte == 0 | off-by-one in length field write |
| `ssidLen=0`: return value == fixed header size | size calculation not subtracting empty SSID |

#### `test_proximity` — `lookupOui` gaps

| Test | Failure mode caught |
|---|---|
| Not found: `vendorOut[0]=='\0'` AND `*threatOut==false` | uninitialized output on miss |
| `entryCount=0`: same postconditions | loop bounds `<` vs `<=` |
| Found at index 1 (first entry is different OUI) | early-exit bug |
| Found with `threat=true`: `*threatOut==true` | threat flag never set |

#### `test_wifi_scan` — `compareApByRssi`

| Test | Failure mode caught |
|---|---|
| `rssi=-80` vs `rssi=-60` → result `> 0` | sign inversion giving ascending sort |

### Total new tests: 13

All run under `[env:native_tests]` (native platform, no hardware).

---

## Delivery order

1. `lib/logic/subghz_logic` + `test/test_subghz` (logic-first, tested before hardware wire-up)
2. `ISubGhzRadio::isPresent()` + `CC1101Radio` + `MockSubGhzRadio` (HAL layer)
3. `main.cpp` setup wire-up + `subghz.cpp` UI banner (BUG-006 complete)
4. `tools/gen_rgb565.py` + `include/hal9000_bg_rgb.h` (generate RGB565 array)
5. `IDisplay::pushImage()` + `TftDisplay` + `MockDisplay` (HAL extension)
6. `src/main.cpp:371` swap + `test_hal9000_assets` regression guard (BUG-005 complete)
7. Coverage gap tests: `test_theme`, `test_wifi_beacon`, `test_proximity`, `test_wifi_scan` (TECH-001 complete)
