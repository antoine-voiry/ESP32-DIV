# TECH-001 Logic Extraction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extract 6 pure-logic functions from `src/` into `lib/logic/` and add 23 adversarial unit tests, raising the native test suite from 153 to 176 tests.

**Architecture:** Each extraction follows TDD — write tests against the new lib/logic/ location first (fails to compile), add the implementation to lib/logic/, delete from src/, verify hardware build still passes. All three groups are independent tasks.

**Tech Stack:** PlatformIO 6.x, Unity test framework, C++17, ESP32-WROOM-32.

## Global Constraints

- `lib/logic/` files: zero Arduino/ESP32 headers — only `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`, `<string.h>`
- Buttons via PCF8574 expander 0x20 — never direct GPIO 3/4/6/7
- `-Wl,-zmuldefs` mandatory in `[env:esp32_v1_hardware]` build_flags; absent from `[env:native_tests]`
- arduinoFFT pinned to 1.6.2 — do not bump
- Every test targets a named, plausible implementation failure mode (comment required)
- Build: `platformio run` (hardware env). Tests: `platformio test -e native_tests`. Both must pass after every task.
- One atomic commit per task

---

## File Map

**Task 1 — Group A (proximity):**
- Modify: `lib/logic/proximity_logic.h` — add `PROX_COLOR_*` constants + 3 inline functions
- Modify: `src/proximity_wand.cpp` — delete 3 static function definitions
- Modify: `test/test_proximity/test_main.cpp` — add 13 tests + RUN_TEST registrations

**Task 2 — Group B (wifi_packet):**
- Create: `lib/logic/wifi_packet_logic.h`
- Create: `lib/logic/wifi_packet_logic.cpp`
- Modify: `src/wifi.cpp` — replace `getMultiplicator()` body + add include
- Create: `test/test_wifi_packet/test_main.cpp`

**Task 3 — Group C (wifi display strength):**
- Modify: `lib/logic/battery_logic.h` — add declaration
- Modify: `lib/logic/battery_logic.cpp` — add implementation
- Modify: `src/utils.cpp` — replace 5-line inline block
- Modify: `test/test_battery/test_main.cpp` — add 5 tests + RUN_TEST registrations

---

### Task 1: Group A — proximity_logic additions

**Files:**
- Modify: `lib/logic/proximity_logic.h`
- Modify: `src/proximity_wand.cpp`
- Test: `test/test_proximity/test_main.cpp`

**Interfaces:**
- Consumes: `RssiZone` enum already defined in `lib/logic/proximity_logic.h` (ZONE_NONE=0, ZONE_LOW=1, ZONE_MODERATE=2, ZONE_CRITICAL=3)
- Produces: `int8_t hitsToRssiProxy(uint8_t hits)`, `uint16_t zoneColor(RssiZone z)`, `const char* zoneLabel(RssiZone z)`, constants `PROX_COLOR_NONE=0x4208`, `PROX_COLOR_LOW=0x07E0`, `PROX_COLOR_MODERATE=0xFFE0`, `PROX_COLOR_CRITICAL=0xF800`

- [ ] **Step 1: Add 13 failing tests to `test/test_proximity/test_main.cpp`**

Insert the following test functions before the `int main(` line:

```cpp
// ── hitsToRssiProxy adversarial tests ────────────────────────────────────────

// Failure mode: off-by-one skips hits==0 branch (returns wrong floor)
void test_hits_zero_returns_floor() {
    TEST_ASSERT_EQUAL_INT8(-100, hitsToRssiProxy(0));
}
// Failure mode: first non-zero case skipped, falls through to -100
void test_hits_one_returns_near_floor() {
    TEST_ASSERT_EQUAL_INT8(-80, hitsToRssiProxy(1));
}
// Failure mode: hits==4 falls through to -40 (penultimate if missing)
void test_hits_four_penultimate() {
    TEST_ASSERT_EQUAL_INT8(-47, hitsToRssiProxy(4));
}
// Failure mode: 5+ case not reached; implementation stops at 4
void test_hits_five_returns_max() {
    TEST_ASSERT_EQUAL_INT8(-40, hitsToRssiProxy(5));
}
// Failure mode: large value triggers UB or wrong return
void test_hits_large_saturates() {
    TEST_ASSERT_EQUAL_INT8(-40, hitsToRssiProxy(255));
}

// ── zoneColor adversarial tests ───────────────────────────────────────────────

// Failure mode: swapped case order in switch (ZONE_NONE returns wrong color)
void test_zone_color_none() {
    uint16_t result = 0xFFFF;  // poison: any wrong color caught
    result = zoneColor(ZONE_NONE);
    TEST_ASSERT_EQUAL_HEX16(PROX_COLOR_NONE, result);
}
void test_zone_color_low() {
    TEST_ASSERT_EQUAL_HEX16(PROX_COLOR_LOW, zoneColor(ZONE_LOW));
}
void test_zone_color_moderate() {
    TEST_ASSERT_EQUAL_HEX16(PROX_COLOR_MODERATE, zoneColor(ZONE_MODERATE));
}
void test_zone_color_critical() {
    TEST_ASSERT_EQUAL_HEX16(PROX_COLOR_CRITICAL, zoneColor(ZONE_CRITICAL));
}

// ── zoneLabel adversarial tests ───────────────────────────────────────────────

// Failure mode: swapped label strings (ZONE_NONE returns "DISTANT" etc.)
void test_zone_label_none() {
    const char* result = "WRONG";  // poison to catch uninitialized/wrong return
    result = zoneLabel(ZONE_NONE);
    TEST_ASSERT_EQUAL_STRING("NO SIGNAL", result);
}
void test_zone_label_low() {
    TEST_ASSERT_EQUAL_STRING("DISTANT", zoneLabel(ZONE_LOW));
}
void test_zone_label_moderate() {
    TEST_ASSERT_EQUAL_STRING("APPROACHING", zoneLabel(ZONE_MODERATE));
}
void test_zone_label_critical() {
    TEST_ASSERT_EQUAL_STRING("CLOSE RANGE", zoneLabel(ZONE_CRITICAL));
}
```

Then add these 13 `RUN_TEST` calls inside `main()` before `return UNITY_END();`:

```cpp
    RUN_TEST(test_hits_zero_returns_floor);
    RUN_TEST(test_hits_one_returns_near_floor);
    RUN_TEST(test_hits_four_penultimate);
    RUN_TEST(test_hits_five_returns_max);
    RUN_TEST(test_hits_large_saturates);
    RUN_TEST(test_zone_color_none);
    RUN_TEST(test_zone_color_low);
    RUN_TEST(test_zone_color_moderate);
    RUN_TEST(test_zone_color_critical);
    RUN_TEST(test_zone_label_none);
    RUN_TEST(test_zone_label_low);
    RUN_TEST(test_zone_label_moderate);
    RUN_TEST(test_zone_label_critical);
```

- [ ] **Step 2: Verify tests fail to compile**

```bash
platformio test -e native_tests --filter test_proximity 2>&1 | grep -E "error:|FAILED|undefined"
```

Expected: compile error — `hitsToRssiProxy`, `zoneColor`, `zoneLabel`, `PROX_COLOR_*` not declared.

- [ ] **Step 3: Add constants and functions to `lib/logic/proximity_logic.h`**

Insert the following block after `ewmaRssi` (after line 49) and before the OUI lookup comment block:

```cpp
// ── Proximity zone color constants (RGB565) ───────────────────────────────────
// Hex values match PW_DIM / PW_GREEN / PW_YELLOW / PW_RED in proximity_wand.h.
// Defined here so zoneColor() needs no TFT or Arduino headers.
static constexpr uint16_t PROX_COLOR_NONE     = 0x4208;  // dark grey — no signal
static constexpr uint16_t PROX_COLOR_LOW      = 0x07E0;  // green — distant
static constexpr uint16_t PROX_COLOR_MODERATE = 0xFFE0;  // yellow — approaching
static constexpr uint16_t PROX_COLOR_CRITICAL = 0xF800;  // red — close range

// ── BLE hit-count → synthetic dBm proxy ──────────────────────────────────────
// Maps packet hit-count in one PROX_RADIO_MS window to a synthetic dBm value.
// 0 hits = -100 dBm (noise floor); 5+ hits = -40 dBm (very close).
inline int8_t hitsToRssiProxy(uint8_t hits) {
    if (hits == 0) return -100;
    if (hits == 1) return -80;
    if (hits == 2) return -65;
    if (hits == 3) return -55;
    if (hits == 4) return -47;
    return -40;
}

// ── Zone → display color ──────────────────────────────────────────────────────
inline uint16_t zoneColor(RssiZone z) {
    switch (z) {
        case ZONE_NONE:     return PROX_COLOR_NONE;
        case ZONE_LOW:      return PROX_COLOR_LOW;
        case ZONE_MODERATE: return PROX_COLOR_MODERATE;
        case ZONE_CRITICAL: return PROX_COLOR_CRITICAL;
    }
    return PROX_COLOR_NONE;
}

// ── Zone → display label ──────────────────────────────────────────────────────
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

- [ ] **Step 4: Verify 13 new tests pass**

```bash
platformio test -e native_tests --filter test_proximity 2>&1 | tail -5
```

Expected: all proximity tests pass (previous 27 + 13 new = 40 total).

- [ ] **Step 5: Delete the three static definitions from `src/proximity_wand.cpp`**

Delete this block (the `hitsToRssiProxy` function, lines ~110–117):

```cpp
static int8_t hitsToRssiProxy(uint8_t hits) {
    if (hits == 0) return -100;
    if (hits == 1) return -80;
    if (hits == 2) return -65;
    if (hits == 3) return -55;
    if (hits == 4) return -47;
    return -40;  // 5+
}
```

Delete this block (`zoneColor`, lines ~164–172):

```cpp
static uint16_t zoneColor(RssiZone z) {
    switch (z) {
        case ZONE_NONE:     return PW_DIM;
        case ZONE_LOW:      return PW_GREEN;
        case ZONE_MODERATE: return PW_YELLOW;
        case ZONE_CRITICAL: return PW_RED;
    }
    return PW_DIM;
}
```

Delete this block (`zoneLabel`, lines ~174–182):

```cpp
static const char* zoneLabel(RssiZone z) {
    switch (z) {
        case ZONE_NONE:     return "NO SIGNAL";
        case ZONE_LOW:      return "DISTANT";
        case ZONE_MODERATE: return "APPROACHING";
        case ZONE_CRITICAL: return "CLOSE RANGE";
    }
    return "";
}
```

After deletion, call sites in `proximity_wand.cpp` (lines ~146, ~201, ~247, ~264) resolve to the inline functions in `proximity_logic.h` via the existing `#include "proximity_wand.h"` → `#include "proximity_logic.h"` chain. No call site changes needed.

- [ ] **Step 6: Verify hardware build passes**

```bash
platformio run 2>&1 | tail -5
```

Expected: `esp32_v1_hardware  SUCCESS`.

- [ ] **Step 7: Run full native test suite**

```bash
platformio test -e native_tests 2>&1 | tail -10
```

Expected: all suites pass. Proximity suite now shows 40 tests.

- [ ] **Step 8: Commit**

```bash
git add lib/logic/proximity_logic.h src/proximity_wand.cpp test/test_proximity/test_main.cpp
git commit -m "refactor(TECH-001): extract hitsToRssiProxy/zoneColor/zoneLabel to proximity_logic — 13 tests"
```

---

### Task 2: Group B — wifi_packet_logic

**Files:**
- Create: `lib/logic/wifi_packet_logic.h`
- Create: `lib/logic/wifi_packet_logic.cpp`
- Modify: `src/wifi.cpp`
- Create: `test/test_wifi_packet/test_main.cpp`

**Interfaces:**
- Consumes: nothing from other tasks
- Produces: `double computePacketGraphScale(const uint32_t* counts, size_t len, double displayHeight)`
  - `counts`: pointer to array of packet counts
  - `len`: number of elements in `counts`
  - `displayHeight`: maximum Y pixels available for the graph
  - Returns: `displayHeight / max` when `max > displayHeight`, else `1.0`; `maxVal` initialized to 1 (guards divide-by-zero on all-zero input)

- [ ] **Step 1: Create `test/test_wifi_packet/test_main.cpp`**

```cpp
// test/test_wifi_packet/test_main.cpp
#include <unity.h>
#include "wifi_packet_logic.h"

void setUp()    {}
void tearDown() {}

// computePacketGraphScale — Y-axis scale for packet monitor graph

// Failure mode: divide-by-zero or NaN when all counts are 0 (maxVal must init to 1, not 0)
void test_all_zeros_returns_one() {
    uint32_t counts[5] = {0, 0, 0, 0, 0};
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 1.0, computePacketGraphScale(counts, 5, 320.0));
}

// Failure mode: loop starts at index 1, misses max at index 0
void test_max_at_first_index() {
    uint32_t counts[5] = {640, 0, 0, 0, 0};
    // 320.0 / 640 = 0.5
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.5, computePacketGraphScale(counts, 5, 320.0));
}

// Failure mode: loop stops one short (i < len-1), misses max at last index
void test_max_at_last_index() {
    uint32_t counts[5] = {0, 0, 0, 0, 640};
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.5, computePacketGraphScale(counts, 5, 320.0));
}

// Failure mode: off-by-one in max search when all values are equal
void test_all_equal_above_display_height() {
    uint32_t counts[4] = {1000, 1000, 1000, 1000};
    // 320.0 / 1000 = 0.32
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.32, computePacketGraphScale(counts, 4, 320.0));
}

// Failure mode: single-element array crashes or is skipped
void test_single_element_above_display_height() {
    uint32_t counts[1] = {800};
    // 320.0 / 800 = 0.4
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.4, computePacketGraphScale(counts, 1, 320.0));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_all_zeros_returns_one);
    RUN_TEST(test_max_at_first_index);
    RUN_TEST(test_max_at_last_index);
    RUN_TEST(test_all_equal_above_display_height);
    RUN_TEST(test_single_element_above_display_height);
    return UNITY_END();
}
```

- [ ] **Step 2: Verify tests fail to compile**

```bash
platformio test -e native_tests --filter test_wifi_packet 2>&1 | grep -E "error:|FAILED|undefined"
```

Expected: compile error — `wifi_packet_logic.h` not found.

- [ ] **Step 3: Create `lib/logic/wifi_packet_logic.h`**

```cpp
// lib/logic/wifi_packet_logic.h
// Pure C++ — no Arduino or hardware headers.
#pragma once
#include <stdint.h>
#include <stddef.h>

// Returns the Y-axis scale factor for the packet monitor bar graph.
// Scans counts[0..len-1] for the maximum value.
// maxVal initialized to 1 (not 0): prevents divide-by-zero on all-zero input
// and keeps scale at 1.0 when all counts fit within displayHeight.
// Returns displayHeight/max when max > displayHeight, else 1.0.
double computePacketGraphScale(const uint32_t* counts, size_t len, double displayHeight);
```

- [ ] **Step 4: Create `lib/logic/wifi_packet_logic.cpp`**

```cpp
// lib/logic/wifi_packet_logic.cpp
#include "wifi_packet_logic.h"

double computePacketGraphScale(const uint32_t* counts, size_t len, double displayHeight) {
    uint32_t maxVal = 1;
    for (size_t i = 0; i < len; i++) {
        if (counts[i] > maxVal) maxVal = counts[i];
    }
    if ((double)maxVal > displayHeight) return displayHeight / (double)maxVal;
    return 1.0;
}
```

- [ ] **Step 5: Verify 5 new tests pass**

```bash
platformio test -e native_tests --filter test_wifi_packet 2>&1 | tail -5
```

Expected: `5 Tests 0 Failures 0 Ignored`.

- [ ] **Step 6: Replace `getMultiplicator()` body in `src/wifi.cpp`**

First, add the include near the top of `src/wifi.cpp` with the other logic includes (search for any existing `#include "wifi_scan_logic.h"` or similar to find the right location):

```cpp
#include "wifi_packet_logic.h"
```

Then replace the body of `getMultiplicator()` (currently at line ~207). The function signature stays unchanged; only the body is replaced:

Old body:
```cpp
double getMultiplicator() {
  uint32_t maxVal = 1;
  for (int i = 0; i < MAX_X; i++) {
    if (pkts[i] > maxVal) maxVal = pkts[i];
  }
  if (maxVal > MAX_Y) return (double)MAX_Y / (double)maxVal;
  else return 1;
}
```

New body:
```cpp
double getMultiplicator() {
    return computePacketGraphScale(pkts, MAX_X, (double)MAX_Y);
}
```

`pkts` is `uint32_t pkts[MAX_X]` (line 75), `MAX_X=240`, `MAX_Y=320` — all defined in `wifi.cpp` scope.

- [ ] **Step 7: Verify hardware build passes**

```bash
platformio run 2>&1 | tail -5
```

Expected: `esp32_v1_hardware  SUCCESS`.

- [ ] **Step 8: Run full native test suite**

```bash
platformio test -e native_tests 2>&1 | tail -10
```

Expected: all suites pass. Count is now 153 + 13 (Task 1) + 5 (Task 2) = 171 tests.

- [ ] **Step 9: Commit**

```bash
git add lib/logic/wifi_packet_logic.h lib/logic/wifi_packet_logic.cpp \
        src/wifi.cpp test/test_wifi_packet/test_main.cpp
git commit -m "refactor(TECH-001): extract getMultiplicator to wifi_packet_logic — 5 tests"
```

---

### Task 3: Group C — computeWifiDisplayStrength

**Files:**
- Modify: `lib/logic/battery_logic.h`
- Modify: `lib/logic/battery_logic.cpp`
- Modify: `src/utils.cpp`
- Test: `test/test_battery/test_main.cpp`

**Interfaces:**
- Consumes: `computeWifiStrength(int rssi)` already in `battery_logic.cpp` — maps rssi∈[-100,-30] → [0,100], clamped. Formula: `(rssi - (-100)) * 100 / (-30 - (-100))`. Examples: rssi=-50 → 71, rssi=-100 → 0, rssi=-30 → 100.
- Produces: `int computeWifiDisplayStrength(bool isConnected, bool isAp, int rssi)` — 3-branch router: connected→RSSI path, AP→100, neither→0.

- [ ] **Step 1: Add 5 failing tests to `test/test_battery/test_main.cpp`**

Insert the following test functions before `int main(`:

```cpp
// ── computeWifiDisplayStrength adversarial tests ──────────────────────────────

// Failure mode: connected branch not taken — returns 0 for any connected state
void test_wifi_display_connected_good_signal() {
    // computeWifiStrength(-50) = ((-50)-(-100))*100/70 = 5000/70 = 71
    TEST_ASSERT_EQUAL_INT(71, computeWifiDisplayStrength(true, false, -50));
}

// Failure mode: RSSI floor not applied — returns positive value at -100 dBm
void test_wifi_display_connected_floor_signal() {
    // computeWifiStrength(-100) = 0
    TEST_ASSERT_EQUAL_INT(0, computeWifiDisplayStrength(true, false, -100));
}

// Failure mode: AP branch not taken — returns 0 instead of 100 in AP mode
void test_wifi_display_ap_mode_returns_full() {
    TEST_ASSERT_EQUAL_INT(100, computeWifiDisplayStrength(false, true, -80));
}

// Failure mode: default branch does not return 0 (returns garbage or previous stack value)
void test_wifi_display_neither_returns_zero() {
    TEST_ASSERT_EQUAL_INT(0, computeWifiDisplayStrength(false, false, -80));
}

// Failure mode: isAp check runs before isConnected check — returns 100 instead of RSSI value
void test_wifi_display_connected_wins_over_ap() {
    TEST_ASSERT_EQUAL_INT(71, computeWifiDisplayStrength(true, true, -50));
}
```

Add these 5 `RUN_TEST` calls inside `main()` before `return UNITY_END();`:

```cpp
    RUN_TEST(test_wifi_display_connected_good_signal);
    RUN_TEST(test_wifi_display_connected_floor_signal);
    RUN_TEST(test_wifi_display_ap_mode_returns_full);
    RUN_TEST(test_wifi_display_neither_returns_zero);
    RUN_TEST(test_wifi_display_connected_wins_over_ap);
```

- [ ] **Step 2: Verify tests fail to compile**

```bash
platformio test -e native_tests --filter test_battery 2>&1 | grep -E "error:|undefined"
```

Expected: compile error — `computeWifiDisplayStrength` not declared.

- [ ] **Step 3: Add declaration to `lib/logic/battery_logic.h`**

Append to `battery_logic.h` after `computeTempStatus`:

```cpp
// Returns display WiFi strength 0–100.
// Connected → maps rssi via computeWifiStrength; AP mode → 100; neither → 0.
// isConnected takes priority over isAp.
int computeWifiDisplayStrength(bool isConnected, bool isAp, int rssi);
```

- [ ] **Step 4: Add implementation to `lib/logic/battery_logic.cpp`**

Append to `battery_logic.cpp` after `computeTempStatus`:

```cpp
int computeWifiDisplayStrength(bool isConnected, bool isAp, int rssi) {
    if (isConnected) return computeWifiStrength(rssi);
    if (isAp)        return 100;
    return 0;
}
```

- [ ] **Step 5: Verify 5 new tests pass**

```bash
platformio test -e native_tests --filter test_battery 2>&1 | tail -5
```

Expected: all battery tests pass (previous 22 + 5 new = 27 total).

- [ ] **Step 6: Replace inline block in `src/utils.cpp`**

`battery_logic.h` is already included at line 8 of `utils.cpp` — no include change needed.

Replace this block in `drawStatusBar()` (lines ~187–192):

```cpp
    int wifiStrength = 0;
    wifi_mode_t wifiMode = WiFi.getMode();
    if (WiFi.status() == WL_CONNECTED) {
        wifiStrength = computeWifiStrength(WiFi.RSSI());
    } else if (wifiMode == WIFI_AP || wifiMode == WIFI_AP_STA) {
        wifiStrength = 100;
    }
```

With:

```cpp
    wifi_mode_t wifiMode = WiFi.getMode();
    int wifiStrength = computeWifiDisplayStrength(
        WiFi.status() == WL_CONNECTED,
        wifiMode == WIFI_AP || wifiMode == WIFI_AP_STA,
        WiFi.RSSI());
```

- [ ] **Step 7: Verify hardware build passes**

```bash
platformio run 2>&1 | tail -5
```

Expected: `esp32_v1_hardware  SUCCESS`.

- [ ] **Step 8: Run full native test suite**

```bash
platformio test -e native_tests 2>&1 | tail -10
```

Expected: all suites pass. Final count: 153 + 13 + 5 + 5 = 176 tests.

- [ ] **Step 9: Commit**

```bash
git add lib/logic/battery_logic.h lib/logic/battery_logic.cpp \
        src/utils.cpp test/test_battery/test_main.cpp
git commit -m "refactor(TECH-001): extract computeWifiDisplayStrength to battery_logic — 5 tests"
```
