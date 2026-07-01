# Design: TECH-001 Logic Extraction — proximity, wifi_packet, wifi display strength

**Date:** 2026-07-01
**Parent:** TECH-001 (unit test coverage)
**Approach:** Option 1 — hex color literals in lib/logic/ (no TFT header dependency)

---

## Scope

Extract 6 pure-logic functions from `src/` into `lib/logic/` so they can be
unit-tested natively. Three groups, delivered in order:

| Group | Functions | Source file | Destination |
|-------|-----------|-------------|-------------|
| A | `hitsToRssiProxy`, `zoneColor`, `zoneLabel` | `src/proximity_wand.cpp` | `lib/logic/proximity_logic.h` (inline) |
| B | `computePacketGraphScale` (renamed from `getMultiplicator`) | `src/wifi.cpp` | `lib/logic/wifi_packet_logic.h/.cpp` (new) |
| C | `computeWifiDisplayStrength` | `src/utils.cpp` (buried in `drawStatusBar`) | `lib/logic/battery_logic.h/.cpp` |

**Not in scope:** `updateActiveSubmenu` (menu routing — requires global decoupling
redesign, deferred), display-drawing functions (hardware-coupled by nature).

---

## Group A — proximity_logic additions

### Problem

`hitsToRssiProxy`, `zoneColor`, `zoneLabel` are `static` functions defined in
`src/proximity_wand.cpp` alongside TFT draw calls. Their 13 branches (5-branch
if-chain + two 4-case switches with default) are invisible to the native test
runner. `RssiZone` is already in `lib/logic/proximity_logic.h`; only the
functions are missing.

### Color constants

`zoneColor()` returns `uint16_t` color values. The `PW_*` constants that define
those values live in `include/proximity_wand.h`, which includes `<Arduino.h>` —
blocked from `lib/logic/`. Solution: define named hex constants directly in
`proximity_logic.h`:

```cpp
static constexpr uint16_t PROX_COLOR_NONE     = 0x4208;  // dark grey
static constexpr uint16_t PROX_COLOR_LOW      = 0x07E0;  // green
static constexpr uint16_t PROX_COLOR_MODERATE = 0xFFE0;  // yellow
static constexpr uint16_t PROX_COLOR_CRITICAL = 0xF800;  // red
```

Values match `PW_DIM / PW_GREEN / PW_YELLOW / PW_RED` in `proximity_wand.h`.
`proximity_wand.cpp` call sites use the function's return value unchanged — no
rename required in the hardware wrapper.

### Changes

**`lib/logic/proximity_logic.h`** — add after `rssiToBarWidth`:
```cpp
static constexpr uint16_t PROX_COLOR_NONE     = 0x4208;
static constexpr uint16_t PROX_COLOR_LOW      = 0x07E0;
static constexpr uint16_t PROX_COLOR_MODERATE = 0xFFE0;
static constexpr uint16_t PROX_COLOR_CRITICAL = 0xF800;

inline int8_t hitsToRssiProxy(uint8_t hits) {
    if (hits == 0) return -100;
    if (hits == 1) return -80;
    if (hits == 2) return -65;
    if (hits == 3) return -55;
    if (hits == 4) return -47;
    return -40;
}

inline uint16_t zoneColor(RssiZone z) {
    switch (z) {
        case ZONE_NONE:     return PROX_COLOR_NONE;
        case ZONE_LOW:      return PROX_COLOR_LOW;
        case ZONE_MODERATE: return PROX_COLOR_MODERATE;
        case ZONE_CRITICAL: return PROX_COLOR_CRITICAL;
    }
    return PROX_COLOR_NONE;
}

inline const char* zoneLabel(RssiZone z) {
    switch (z) {
        case ZONE_NONE:     return "NO SIGNAL";
        case ZONE_LOW:      return "DISTANT";
        case ZONE_MODERATE: return "APPROACHING";
        case ZONE_CRITICAL: return "CLOSE RANGE";
    }
    return "";
}
```

**`src/proximity_wand.cpp`** — delete the three `static` function definitions
(lines ~110–182). `proximity_wand.h` already includes `proximity_logic.h`, so
the functions are available at the call sites with no include change.

**`test/test_proximity/test_main.cpp`** — 13 new adversarial tests:

| Test | Failure mode caught |
|------|---------------------|
| `hits=0 → -100` | boundary not taken (off-by-one at 0) |
| `hits=1 → -80` | first non-zero case skipped |
| `hits=4 → -47` | penultimate case falling through to -40 |
| `hits=5 → -40` | 5+ case not reached |
| `hits=255 → -40` | large value not handled, crash or wrong return |
| `zoneColor(ZONE_NONE) == PROX_COLOR_NONE` | pre-poison output; catch swapped case order |
| `zoneColor(ZONE_LOW) == PROX_COLOR_LOW` | idem |
| `zoneColor(ZONE_MODERATE) == PROX_COLOR_MODERATE` | idem |
| `zoneColor(ZONE_CRITICAL) == PROX_COLOR_CRITICAL` | idem |
| `zoneLabel(ZONE_NONE) == "NO SIGNAL"` | pre-poison to "WRONG"; catch swapped labels |
| `zoneLabel(ZONE_LOW) == "DISTANT"` | idem |
| `zoneLabel(ZONE_MODERATE) == "APPROACHING"` | idem |
| `zoneLabel(ZONE_CRITICAL) == "CLOSE RANGE"` | idem |

---

## Group B — wifi_packet_logic (new)

### Problem

`getMultiplicator()` (`src/wifi.cpp:207`) computes the Y-axis scale factor for
the packet monitor graph — scans `pkts[MAX_X]` for the max value, returns a
`double` scale factor to fit the bar graph within `MAX_Y` display pixels. Zero
hardware calls. Zero tests. The function uses `pkts[]` and `MAX_X`/`MAX_Y` as
globals, preventing extraction as-is; the fix is to accept them as parameters.

### Signature

```cpp
// lib/logic/wifi_packet_logic.h
double computePacketGraphScale(const uint32_t* counts, size_t len, double displayHeight);
```

- Scans `counts[0..len-1]` for max
- `maxVal` initialized to `1` (existing behaviour: avoids divide-by-zero, scale
  never exceeds displayHeight when all counts are 0)
- If `maxVal > displayHeight`: returns `displayHeight / maxVal`
- Otherwise: returns `1.0`

**`src/wifi.cpp`** — replace `getMultiplicator()` body with:
```cpp
double getMultiplicator() {
    return computePacketGraphScale(pkts, MAX_X, (double)MAX_Y);
}
```
The wrapper function name stays unchanged — no other call site changes needed.

### Tests (`test/test_wifi_packet/test_main.cpp` — new suite)

| Test | Failure mode caught |
|------|---------------------|
| All-zeros, len=MAX_X → 1.0 | divide-by-zero or NaN return |
| Max at index 0, value > MAX_Y → scale < 1.0 | loop starting at index 1, missing element 0 |
| Max at last index, value > MAX_Y → scale < 1.0 | loop stopping one short |
| All equal values > MAX_Y → scale == MAX_Y / value | off-by-one in max search |
| len=1, single value > MAX_Y → correct scale | boundary len=1 |

---

## Group C — computeWifiDisplayStrength

### Problem

`drawStatusBar()` (`src/utils.cpp:187–193`) contains a 3-branch WiFi signal
strength selection:

```cpp
int wifiStrength = 0;
wifi_mode_t wifiMode = WiFi.getMode();
if (WiFi.status() == WL_CONNECTED) {
    wifiStrength = computeWifiStrength(WiFi.RSSI());
} else if (wifiMode == WIFI_AP || wifiMode == WIFI_AP_STA) {
    wifiStrength = 100;
}
```

The hardware resolution (`WiFi.status()`, `WiFi.getMode()`, `WiFi.RSSI()`) is
done at the call site. The decision logic is pure. It belongs in `battery_logic`
alongside `computeWifiStrength` (which it calls).

### Signature

```cpp
// lib/logic/battery_logic.h
int computeWifiDisplayStrength(bool isConnected, bool isAp, int rssi);
```

Implementation:
```cpp
int computeWifiDisplayStrength(bool isConnected, bool isAp, int rssi) {
    if (isConnected) return computeWifiStrength(rssi);
    if (isAp)        return 100;
    return 0;
}
```

**`src/utils.cpp:drawStatusBar()`** — replace inline block with:
```cpp
wifi_mode_t wifiMode = WiFi.getMode();
int wifiStrength = computeWifiDisplayStrength(
    WiFi.status() == WL_CONNECTED,
    wifiMode == WIFI_AP || wifiMode == WIFI_AP_STA,
    WiFi.RSSI());
```

### Tests (added to `test/test_battery/test_main.cpp`)

| Test | Failure mode caught |
|------|---------------------|
| `isConnected=true, rssi=-50` → 71 (via `computeWifiStrength`) | connected branch not taken |
| `isConnected=true, rssi=-100` → 0 | rssi floor not applied |
| `isConnected=false, isAp=true` → 100 | AP branch not taken |
| `isConnected=false, isAp=false` → 0 | default not returning 0 |
| `isConnected=true, isAp=true` → RSSI path wins (not 100) | isAp short-circuit before connection check |

---

## Test totals

| Suite | New tests | Running total after |
|-------|-----------|---------------------|
| `test_proximity` | 13 | +13 |
| `test_wifi_packet` | 5 | +5 (new suite) |
| `test_battery` | 5 | +5 |
| **Total new** | **23** | **153 → 176** |

---

## Global constraints (inherited)

- `lib/logic/` files: zero Arduino/ESP32 headers — only `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`, `<string.h>`
- Buttons via PCF8574 expander 0x20 — never direct GPIO 3/6/7
- `-Wl,-zmuldefs` in `[env:esp32_v1_hardware]` only
- arduinoFFT pinned to 1.6.2
- Every test targets a named failure mode; no padding
- `platformio run` (hardware) + `platformio test -e native_tests` must pass after each task
- Atomic commit per task
