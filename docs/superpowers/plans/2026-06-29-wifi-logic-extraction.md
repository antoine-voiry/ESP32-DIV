# WiFi Logic Extraction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extract pure decision logic from `src/wifi.cpp` into three `lib/logic/` modules — `wifi_frame_logic`, `wifi_scan_logic`, `wifi_beacon_logic` — so that logic is unit-testable on host GCC without Arduino or ESP-IDF headers.

**Architecture:** Each logic module has a header and implementation with zero Arduino/ESP-IDF dependencies (`<stdint.h>`, `<stddef.h>`, `<stdbool.h>`, `<string.h>` only). `src/wifi.cpp` gains three `#include` lines and its call sites are updated to delegate to the logic. Three Unity test suites cover 34 cases total (10 + 11 + 13). The hardware env (`esp32_v1_hardware`) must still compile cleanly after every task.

**Tech Stack:** PlatformIO 6.x, Unity (via `[env:native_tests]`), host GCC, ESP32 Arduino core 2.0.17 (hardware build only)

## Global Constraints

- Zero Arduino/ESP-IDF headers in `lib/logic/` files — only `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`, `<string.h>`
- `src/wifi.cpp` call sites must compile under `[env:esp32_v1_hardware]` after each task
- Buttons read via `pcf.digitalRead()` — PCF8574 channels 6/3/4/5/7 — never `pinMode()`/`digitalRead()` on those numbers
- `-Wl,-zmuldefs` flag must remain in `platformio.ini` `build_flags` for `esp32_v1_hardware`
- arduinoFFT pinned to 1.6.2 — do NOT bump
- Atomic commits: one commit per task (logic files + test + call site changes together)
- Caveman Serial rule: any debug prints are bare token only (`Serial.println("VAL")`)
- `analyzeAp` must replicate original `analyzeNetworks` detection logic exactly, including flagging two hidden networks as each other's evil twin
- Deauther's `compare_ap()` / `qsort` on `wifi_ap_record_t` is NOT replaced — it uses ESP-IDF types incompatible with `WifiApInfo`. `compareApByRssi` is added to `wifi_scan_logic` and tested; Deauther wiring is future work.

---

## File Map

**Created:**
- `lib/logic/wifi_frame_logic.h` — frame predicates + channel wrap declarations
- `lib/logic/wifi_frame_logic.cpp` — 3-function implementation
- `lib/logic/wifi_scan_logic.h` — `WifiApInfo`, `NetworkAnomaly` structs + analyzeAp + compareApByRssi
- `lib/logic/wifi_scan_logic.cpp` — 2-function implementation
- `lib/logic/wifi_beacon_logic.h` — channelDown/Up, toggleAttack, buildBeaconTlv declarations
- `lib/logic/wifi_beacon_logic.cpp` — 4-function implementation
- `test/test_wifi_frame/test_main.cpp` — 10 Unity tests
- `test/test_wifi_scan/test_main.cpp` — 11 Unity tests
- `test/test_wifi_beacon/test_main.cpp` — 13 Unity tests

**Modified:**
- `src/wifi.cpp` — add 3 includes, update 6 call sites (Tasks 1–3)

---

### Task 1: wifi_frame_logic — frame predicates and channel wrap

**Files:**
- Create: `lib/logic/wifi_frame_logic.h`
- Create: `lib/logic/wifi_frame_logic.cpp`
- Create: `test/test_wifi_frame/test_main.cpp`
- Modify: `src/wifi.cpp` (3 call sites + 1 include)

**Interfaces:**
- Produces:
  - `bool isDeauthFrame(uint8_t frameType)` — returns `frameType == 0xC0`
  - `bool isDisassocFrame(uint8_t frameType)` — returns `frameType == 0xA0`
  - `uint8_t wrapChannel(uint8_t ch, uint8_t minCh, uint8_t maxCh)` — returns `minCh` if out of range, else `ch`

---

- [ ] **Step 1: Write the failing tests**

Create `test/test_wifi_frame/test_main.cpp`:

```cpp
#include <unity.h>
#include "wifi_frame_logic.h"

void setUp()    {}
void tearDown() {}

void test_isDeauth_0xC0()    { TEST_ASSERT_TRUE(isDeauthFrame(0xC0)); }
void test_isDeauth_other()   { TEST_ASSERT_FALSE(isDeauthFrame(0xA0)); }
void test_isDeauth_zero()    { TEST_ASSERT_FALSE(isDeauthFrame(0x00)); }
void test_isDisassoc_0xA0()  { TEST_ASSERT_TRUE(isDisassocFrame(0xA0)); }
void test_isDisassoc_other() { TEST_ASSERT_FALSE(isDisassocFrame(0xC0)); }
void test_wrap_in_range()    { TEST_ASSERT_EQUAL_UINT8(6,  wrapChannel(6,  1, 13)); }
void test_wrap_above_max()   { TEST_ASSERT_EQUAL_UINT8(1,  wrapChannel(14, 1, 13)); }
void test_wrap_below_min()   { TEST_ASSERT_EQUAL_UINT8(1,  wrapChannel(0,  1, 13)); }
void test_wrap_at_min()      { TEST_ASSERT_EQUAL_UINT8(1,  wrapChannel(1,  1, 13)); }
void test_wrap_at_max()      { TEST_ASSERT_EQUAL_UINT8(13, wrapChannel(13, 1, 13)); }

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_isDeauth_0xC0);
    RUN_TEST(test_isDeauth_other);
    RUN_TEST(test_isDeauth_zero);
    RUN_TEST(test_isDisassoc_0xA0);
    RUN_TEST(test_isDisassoc_other);
    RUN_TEST(test_wrap_in_range);
    RUN_TEST(test_wrap_above_max);
    RUN_TEST(test_wrap_below_min);
    RUN_TEST(test_wrap_at_min);
    RUN_TEST(test_wrap_at_max);
    return UNITY_END();
}
```

- [ ] **Step 2: Run to confirm they fail**

```bash
pio test -e native_tests -f test_wifi_frame 2>&1 | tail -10
```

Expected: compile error `wifi_frame_logic.h: No such file or directory`

- [ ] **Step 3: Create `lib/logic/wifi_frame_logic.h`**

```cpp
#pragma once
#include <stdint.h>

bool    isDeauthFrame(uint8_t frameType);
bool    isDisassocFrame(uint8_t frameType);
uint8_t wrapChannel(uint8_t ch, uint8_t minCh, uint8_t maxCh);
```

- [ ] **Step 4: Create `lib/logic/wifi_frame_logic.cpp`**

```cpp
#include "wifi_frame_logic.h"

bool isDeauthFrame(uint8_t frameType)   { return frameType == 0xC0; }
bool isDisassocFrame(uint8_t frameType) { return frameType == 0xA0; }

uint8_t wrapChannel(uint8_t ch, uint8_t minCh, uint8_t maxCh) {
    if (ch < minCh || ch > maxCh) return minCh;
    return ch;
}
```

- [ ] **Step 5: Run tests — verify 10 pass**

```bash
pio test -e native_tests -f test_wifi_frame 2>&1 | tail -5
```

Expected:
```
10 Tests 0 Failures 0 Ignored
OK
```

- [ ] **Step 6: Add include and update call sites in `src/wifi.cpp`**

Add the include immediately after line 1 (`#include "wificonfig.h"`):

```cpp
#include "wificonfig.h"
#include "wifi_frame_logic.h"
```

**Call site 1 — PacketMonitor `wifi_promiscuous()` (line ~217):**

```cpp
// BEFORE
if (type == WIFI_PKT_MGMT && (pkt->payload[0] == 0xA0 || pkt->payload[0] == 0xC0 )) deauths++;

// AFTER
if (type == WIFI_PKT_MGMT && (isDisassocFrame(pkt->payload[0]) || isDeauthFrame(pkt->payload[0]))) deauths++;
```

Locate with: `grep -n "0xA0 || pkt->payload" src/wifi.cpp`

**Call site 2 — DeauthDetect `snifferCallback()` (line ~1076):**

```cpp
// BEFORE
if (frameType == 0xC0) {

// AFTER
if (isDeauthFrame(frameType)) {
```

Locate with: `grep -n "frameType == 0xC0" src/wifi.cpp`

**Call site 3 — PacketMonitor `setChannel()` (line ~231):**

```cpp
// BEFORE
if (ch > MAX_CH || ch < 1) ch = 1;

// AFTER
ch = (unsigned int)wrapChannel((uint8_t)ch, 1, (uint8_t)MAX_CH);
```

Locate with: `grep -n "ch > MAX_CH" src/wifi.cpp`

- [ ] **Step 7: Verify hardware env still compiles**

```bash
pio run -e esp32_v1_hardware 2>&1 | tail -5
```

Expected: `[SUCCESS] Took N.N seconds`

- [ ] **Step 8: Run full native suite — verify no regressions**

```bash
pio test -e native_tests 2>&1 | tail -5
```

Expected: `≥ 59 Tests 0 Failures 0 Ignored` (49 existing + 10 new)

- [ ] **Step 9: Commit**

```bash
git add lib/logic/wifi_frame_logic.h lib/logic/wifi_frame_logic.cpp \
        test/test_wifi_frame/test_main.cpp src/wifi.cpp
git commit -m "feat(logic): extract wifi_frame_logic — isDeauthFrame, isDisassocFrame, wrapChannel + 10 tests"
```

---

### Task 2: wifi_scan_logic — network anomaly detection and RSSI comparator

**Files:**
- Create: `lib/logic/wifi_scan_logic.h`
- Create: `lib/logic/wifi_scan_logic.cpp`
- Create: `test/test_wifi_scan/test_main.cpp`
- Modify: `src/wifi.cpp` (refactor `analyzeNetworks` + 1 include)

**Interfaces:**
- Consumes: nothing from Task 1
- Produces:
  - `struct WifiApInfo { char ssid[33]; uint8_t bssid[6]; int8_t rssi; uint8_t channel; }`
  - `struct NetworkAnomaly { bool isHidden; bool isEvilTwin; bool isWeirdChannel; bool isUnderAttack; }`
  - `NetworkAnomaly analyzeAp(const WifiApInfo* ap, const WifiApInfo* allAps, int apCount, int deauthCount)`
  - `int compareApByRssi(const void* a, const void* b)` — descending RSSI, qsort-compatible on `WifiApInfo`

**Note:** Deauther's `compare_ap()` / `qsort(ap_list, ..., compare_ap)` is NOT replaced — `ap_list` uses `wifi_ap_record_t` (ESP-IDF type). `compareApByRssi` is extracted and tested here; wiring to Deauther is future work.

---

- [ ] **Step 1: Write the failing tests**

Create `test/test_wifi_scan/test_main.cpp`:

```cpp
#include <unity.h>
#include "wifi_scan_logic.h"
#include <string.h>

void setUp()    {}
void tearDown() {}

static WifiApInfo makeAp(const char* ssid, uint8_t b0, uint8_t b1, uint8_t b2,
                         uint8_t b3, uint8_t b4, uint8_t b5,
                         int8_t rssi, uint8_t channel) {
    WifiApInfo ap = {};
    strncpy(ap.ssid, ssid, 32);
    ap.ssid[32] = '\0';
    ap.bssid[0]=b0; ap.bssid[1]=b1; ap.bssid[2]=b2;
    ap.bssid[3]=b3; ap.bssid[4]=b4; ap.bssid[5]=b5;
    ap.rssi    = rssi;
    ap.channel = channel;
    return ap;
}

void test_normal_ap() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 0);
    TEST_ASSERT_FALSE(r.isHidden);
    TEST_ASSERT_FALSE(r.isEvilTwin);
    TEST_ASSERT_FALSE(r.isWeirdChannel);
    TEST_ASSERT_FALSE(r.isUnderAttack);
}

void test_hidden_ssid() {
    WifiApInfo ap = makeAp("",0x01,0x02,0x03,0x04,0x05,0x06,-70,6);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 0);
    TEST_ASSERT_TRUE(r.isHidden);
}

void test_evil_twin() {
    WifiApInfo allAps[2];
    allAps[0] = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    allAps[1] = makeAp("Home",0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,-65,6);
    NetworkAnomaly r = analyzeAp(&allAps[0], allAps, 2, 0);
    TEST_ASSERT_TRUE(r.isEvilTwin);
}

void test_no_evil_twin_same_bssid() {
    WifiApInfo allAps[2];
    allAps[0] = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    allAps[1] = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-65,6);
    NetworkAnomaly r = analyzeAp(&allAps[0], allAps, 2, 0);
    TEST_ASSERT_FALSE(r.isEvilTwin);
}

void test_no_evil_twin_single_ap() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 0);
    TEST_ASSERT_FALSE(r.isEvilTwin);
}

void test_weird_channel_14() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,14);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 0);
    TEST_ASSERT_TRUE(r.isWeirdChannel);
}

void test_normal_channel_13() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,13);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 0);
    TEST_ASSERT_FALSE(r.isWeirdChannel);
}

void test_under_attack_6() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 6);
    TEST_ASSERT_TRUE(r.isUnderAttack);
}

void test_not_under_attack_5() {
    WifiApInfo ap = makeAp("Home",0x01,0x02,0x03,0x04,0x05,0x06,-60,6);
    NetworkAnomaly r = analyzeAp(&ap, &ap, 1, 5);
    TEST_ASSERT_FALSE(r.isUnderAttack);
}

void test_compare_higher_rssi_first() {
    WifiApInfo a = {}; a.rssi = -30;
    WifiApInfo b = {}; b.rssi = -80;
    TEST_ASSERT_TRUE(compareApByRssi(&a, &b) < 0);
}

void test_compare_equal_rssi() {
    WifiApInfo a = {}; a.rssi = -50;
    WifiApInfo b = {}; b.rssi = -50;
    TEST_ASSERT_EQUAL_INT(0, compareApByRssi(&a, &b));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_normal_ap);
    RUN_TEST(test_hidden_ssid);
    RUN_TEST(test_evil_twin);
    RUN_TEST(test_no_evil_twin_same_bssid);
    RUN_TEST(test_no_evil_twin_single_ap);
    RUN_TEST(test_weird_channel_14);
    RUN_TEST(test_normal_channel_13);
    RUN_TEST(test_under_attack_6);
    RUN_TEST(test_not_under_attack_5);
    RUN_TEST(test_compare_higher_rssi_first);
    RUN_TEST(test_compare_equal_rssi);
    return UNITY_END();
}
```

- [ ] **Step 2: Run to confirm they fail**

```bash
pio test -e native_tests -f test_wifi_scan 2>&1 | tail -5
```

Expected: compile error `wifi_scan_logic.h: No such file or directory`

- [ ] **Step 3: Create `lib/logic/wifi_scan_logic.h`**

```cpp
#pragma once
#include <stdint.h>
#include <stdbool.h>

struct WifiApInfo {
    char    ssid[33];   // null-terminated; ssid[0]=='\0' means hidden
    uint8_t bssid[6];
    int8_t  rssi;       // dBm
    uint8_t channel;
};

struct NetworkAnomaly {
    bool isHidden;       // ssid[0] == '\0'
    bool isEvilTwin;     // same SSID, different BSSID in allAps[]
    bool isWeirdChannel; // channel > 13
    bool isUnderAttack;  // deauthCount > 5
};

NetworkAnomaly analyzeAp(const WifiApInfo* ap,
                         const WifiApInfo* allAps, int apCount,
                         int deauthCount);

int compareApByRssi(const void* a, const void* b);
```

- [ ] **Step 4: Create `lib/logic/wifi_scan_logic.cpp`**

```cpp
#include "wifi_scan_logic.h"
#include <string.h>

NetworkAnomaly analyzeAp(const WifiApInfo* ap,
                         const WifiApInfo* allAps, int apCount,
                         int deauthCount) {
    NetworkAnomaly result = {false, false, false, false};
    result.isHidden      = (ap->ssid[0] == '\0');
    result.isWeirdChannel = (ap->channel > 13);
    result.isUnderAttack  = (deauthCount > 5);

    for (int j = 0; j < apCount; j++) {
        if (memcmp(ap->bssid, allAps[j].bssid, 6) == 0) continue;
        if (strcmp(ap->ssid, allAps[j].ssid) == 0) {
            result.isEvilTwin = true;
            break;
        }
    }
    return result;
}

int compareApByRssi(const void* a, const void* b) {
    const WifiApInfo* ap1 = (const WifiApInfo*)a;
    const WifiApInfo* ap2 = (const WifiApInfo*)b;
    return (int)ap2->rssi - (int)ap1->rssi;
}
```

- [ ] **Step 5: Run tests — verify 11 pass**

```bash
pio test -e native_tests -f test_wifi_scan 2>&1 | tail -5
```

Expected:
```
11 Tests 0 Failures 0 Ignored
OK
```

- [ ] **Step 6: Add include to `src/wifi.cpp`**

After `#include "wifi_frame_logic.h"` (added in Task 1), add:

```cpp
#include "wifi_scan_logic.h"
```

The top of `src/wifi.cpp` should now read:

```cpp
#include "wificonfig.h"
#include "wifi_frame_logic.h"
#include "wifi_scan_logic.h"
```

- [ ] **Step 7: Refactor `analyzeNetworks()` in `src/wifi.cpp`**

Locate with: `grep -n "void analyzeNetworks" src/wifi.cpp`

Replace the entire function body. The function is in the `DeauthDetect` namespace (around line 1093). The original is 43 lines; the refactored version delegates the detection logic to `analyzeAp` while keeping all hardware I/O (semaphore, `displayPrint`) in the caller:

```cpp
void analyzeNetworks(int n) {
    xSemaphoreTake(tftSemaphore, portMAX_DELAY);
    displayPrint("[*] Checking for Suspicious Networks", TFT_CYAN, true);
    xSemaphoreGive(tftSemaphore);

    WifiApInfo allAps[MAX_NETWORKS];
    for (int i = 0; i < n; i++) {
        strncpy(allAps[i].ssid, ssidLists[i].c_str(), 32);
        allAps[i].ssid[32] = '\0';
        memcpy(allAps[i].bssid, macList[i], 6);
        allAps[i].rssi    = 0;
        allAps[i].channel = (uint8_t)WiFi.channel(i);
    }

    for (int i = 0; i < n; i++) {
        checkButtonPress();
        if (exitMode) return;

        NetworkAnomaly flags = analyzeAp(&allAps[i], allAps, n, deauth[i]);

        if (flags.isHidden) {
            xSemaphoreTake(tftSemaphore, portMAX_DELAY);
            displayPrint("[!] Hidden SSID Detected!", TFT_YELLOW, true);
            xSemaphoreGive(tftSemaphore);
        }
        if (flags.isEvilTwin) {
            xSemaphoreTake(tftSemaphore, portMAX_DELAY);
            displayPrint("[!] Evil Twin: " + ssidLists[i], TFT_YELLOW, true);
            xSemaphoreGive(tftSemaphore);
        }
        if (flags.isWeirdChannel) {
            xSemaphoreTake(tftSemaphore, portMAX_DELAY);
            displayPrint("[!] Non-Standard Channel: " + String(allAps[i].channel), TFT_YELLOW, true);
            xSemaphoreGive(tftSemaphore);
        }
        if (flags.isUnderAttack) {
            xSemaphoreTake(tftSemaphore, portMAX_DELAY);
            displayPrint("[!!!] HIGH DEAUTH ATTACK on " + ssidLists[i] + " (" + String(deauth[i]) + " attacks)", ORANGE, true);
            xSemaphoreGive(tftSemaphore);
        }
    }
}
```

- [ ] **Step 8: Verify hardware env still compiles**

```bash
pio run -e esp32_v1_hardware 2>&1 | tail -5
```

Expected: `[SUCCESS] Took N.N seconds`

- [ ] **Step 9: Run full native suite — verify no regressions**

```bash
pio test -e native_tests 2>&1 | tail -5
```

Expected: `≥ 70 Tests 0 Failures 0 Ignored` (59 after Task 1 + 11 new)

- [ ] **Step 10: Commit**

```bash
git add lib/logic/wifi_scan_logic.h lib/logic/wifi_scan_logic.cpp \
        test/test_wifi_scan/test_main.cpp src/wifi.cpp
git commit -m "feat(logic): extract wifi_scan_logic — analyzeAp, compareApByRssi, WifiApInfo + 11 tests"
```

---

### Task 3: wifi_beacon_logic — channel navigation, attack toggle, beacon TLV construction

**Files:**
- Create: `lib/logic/wifi_beacon_logic.h`
- Create: `lib/logic/wifi_beacon_logic.cpp`
- Create: `test/test_wifi_beacon/test_main.cpp`
- Modify: `src/wifi.cpp` (4 call sites + 1 include)

**Interfaces:**
- Consumes: nothing from Tasks 1–2
- Produces:
  - `uint8_t channelDown(uint8_t ch, uint8_t maxCh)` — `(ch == 1) ? maxCh : ch - 1`
  - `uint8_t channelUp(uint8_t ch, uint8_t maxCh)` — `(ch == maxCh) ? 1 : ch + 1`
  - `bool toggleAttack(bool current)` — `!current`
  - `int buildBeaconTlv(uint8_t* packet, size_t bufLen, const char* ssid, uint8_t ssidLen, uint8_t channel)` — writes TLV fields into `packet` starting at byte 37; returns total frame length or -1 on overflow

**Beacon TLV layout** (starting at byte 37 of the pre-initialized 128-byte packet template):
```
byte 37              = ssidLen
bytes 38..37+ssidLen = SSID bytes
ratesOffset = 38+ssidLen:
  [+0]  = 0x01  (Supported Rates tag)
  [+1]  = 0x08  (length)
  [+2]  = 0x82, [+3] = 0x84, [+4] = 0x8b, [+5] = 0x96
  [+6]  = 0x24, [+7] = 0x30, [+8] = 0x48, [+9] = 0x6c
dsOffset = ratesOffset+10:
  [+0]  = 0x03  (DS Parameter tag)
  [+1]  = 0x01  (length)
  [+2]  = channel
return value = dsOffset + 3 = 51 + ssidLen
```

---

- [ ] **Step 1: Write the failing tests**

Create `test/test_wifi_beacon/test_main.cpp`:

```cpp
#include <unity.h>
#include "wifi_beacon_logic.h"
#include <string.h>

void setUp()    {}
void tearDown() {}

void test_channelDown_normal() { TEST_ASSERT_EQUAL_UINT8(5,  channelDown(6,  14)); }
void test_channelDown_wraps()  { TEST_ASSERT_EQUAL_UINT8(14, channelDown(1,  14)); }
void test_channelUp_normal()   { TEST_ASSERT_EQUAL_UINT8(7,  channelUp(6,   14)); }
void test_channelUp_wraps()    { TEST_ASSERT_EQUAL_UINT8(1,  channelUp(14,  14)); }
void test_toggleAttack_false() { TEST_ASSERT_TRUE(toggleAttack(false)); }
void test_toggleAttack_true()  { TEST_ASSERT_FALSE(toggleAttack(true)); }

// buildBeaconTlv: ssidLen=4 ("TEST") → totalLen = 51 + 4 = 55
void test_buildBeaconTlv_size() {
    uint8_t pkt[128] = {};
    int result = buildBeaconTlv(pkt, 128, "TEST", 4, 6);
    TEST_ASSERT_EQUAL_INT(55, result);
}

// ssid bytes written starting at byte 38
void test_buildBeaconTlv_ssid_written() {
    uint8_t pkt[128] = {};
    buildBeaconTlv(pkt, 128, "AB", 2, 6);
    TEST_ASSERT_EQUAL_UINT8('A', pkt[38]);
    TEST_ASSERT_EQUAL_UINT8('B', pkt[39]);
}

// ssid length byte written at byte 37
void test_buildBeaconTlv_ssidlen_byte() {
    uint8_t pkt[128] = {};
    buildBeaconTlv(pkt, 128, "HI", 2, 6);
    TEST_ASSERT_EQUAL_UINT8(2, pkt[37]);
}

// ssidLen=3 → ratesOffset=41; pkt[41] must be Supported Rates tag 0x01
void test_buildBeaconTlv_rates_tag() {
    uint8_t pkt[128] = {};
    buildBeaconTlv(pkt, 128, "ABC", 3, 6);
    TEST_ASSERT_EQUAL_UINT8(0x01, pkt[41]);
}

// ssidLen=3 → dsOffset=51; pkt[51] must be DS Parameter tag 0x03
void test_buildBeaconTlv_ds_tag() {
    uint8_t pkt[128] = {};
    buildBeaconTlv(pkt, 128, "ABC", 3, 6);
    TEST_ASSERT_EQUAL_UINT8(0x03, pkt[51]);
}

// pkt[dsOffset+2] = channel
void test_buildBeaconTlv_ds_channel() {
    uint8_t pkt[128] = {};
    buildBeaconTlv(pkt, 128, "ABC", 3, 11);
    TEST_ASSERT_EQUAL_UINT8(11, pkt[53]);
}

// bufLen too small → returns -1
void test_buildBeaconTlv_overflow() {
    uint8_t pkt[10] = {};
    int result = buildBeaconTlv(pkt, 10, "AB", 2, 6);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_channelDown_normal);
    RUN_TEST(test_channelDown_wraps);
    RUN_TEST(test_channelUp_normal);
    RUN_TEST(test_channelUp_wraps);
    RUN_TEST(test_toggleAttack_false);
    RUN_TEST(test_toggleAttack_true);
    RUN_TEST(test_buildBeaconTlv_size);
    RUN_TEST(test_buildBeaconTlv_ssid_written);
    RUN_TEST(test_buildBeaconTlv_ssidlen_byte);
    RUN_TEST(test_buildBeaconTlv_rates_tag);
    RUN_TEST(test_buildBeaconTlv_ds_tag);
    RUN_TEST(test_buildBeaconTlv_ds_channel);
    RUN_TEST(test_buildBeaconTlv_overflow);
    return UNITY_END();
}
```

- [ ] **Step 2: Run to confirm they fail**

```bash
pio test -e native_tests -f test_wifi_beacon 2>&1 | tail -5
```

Expected: compile error `wifi_beacon_logic.h: No such file or directory`

- [ ] **Step 3: Create `lib/logic/wifi_beacon_logic.h`**

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

uint8_t channelDown(uint8_t ch, uint8_t maxCh);
uint8_t channelUp(uint8_t ch, uint8_t maxCh);
bool    toggleAttack(bool current);
int     buildBeaconTlv(uint8_t* packet, size_t bufLen,
                       const char* ssid, uint8_t ssidLen,
                       uint8_t channel);
```

- [ ] **Step 4: Create `lib/logic/wifi_beacon_logic.cpp`**

```cpp
#include "wifi_beacon_logic.h"
#include <string.h>

uint8_t channelDown(uint8_t ch, uint8_t maxCh) {
    return (ch == 1) ? maxCh : (uint8_t)(ch - 1);
}

uint8_t channelUp(uint8_t ch, uint8_t maxCh) {
    return (ch == maxCh) ? (uint8_t)1 : (uint8_t)(ch + 1);
}

bool toggleAttack(bool current) {
    return !current;
}

int buildBeaconTlv(uint8_t* packet, size_t bufLen,
                   const char* ssid, uint8_t ssidLen,
                   uint8_t channel) {
    int dsOffset = 38 + (int)ssidLen + 10;
    int totalLen = dsOffset + 3;
    if ((size_t)totalLen > bufLen) return -1;

    packet[37] = ssidLen;
    for (int i = 0; i < (int)ssidLen; i++) {
        packet[38 + i] = (uint8_t)ssid[i];
    }

    int ratesOffset = 38 + (int)ssidLen;
    packet[ratesOffset]     = 0x01;
    packet[ratesOffset + 1] = 0x08;
    packet[ratesOffset + 2] = 0x82; packet[ratesOffset + 3] = 0x84;
    packet[ratesOffset + 4] = 0x8b; packet[ratesOffset + 5] = 0x96;
    packet[ratesOffset + 6] = 0x24; packet[ratesOffset + 7] = 0x30;
    packet[ratesOffset + 8] = 0x48; packet[ratesOffset + 9] = 0x6c;

    packet[dsOffset]     = 0x03;
    packet[dsOffset + 1] = 0x01;
    packet[dsOffset + 2] = channel;

    return totalLen;
}
```

- [ ] **Step 5: Run tests — verify 13 pass**

```bash
pio test -e native_tests -f test_wifi_beacon 2>&1 | tail -5
```

Expected:
```
13 Tests 0 Failures 0 Ignored
OK
```

- [ ] **Step 6: Add include to `src/wifi.cpp`**

After `#include "wifi_scan_logic.h"` (added in Task 2), add:

```cpp
#include "wifi_beacon_logic.h"
```

The top of `src/wifi.cpp` should now read:

```cpp
#include "wificonfig.h"
#include "wifi_frame_logic.h"
#include "wifi_scan_logic.h"
#include "wifi_beacon_logic.h"
```

- [ ] **Step 7: Replace BeaconSpammer button handler bodies in `src/wifi.cpp`**

Locate with: `grep -n "void handleLeftButton\|void handleRightButton\|void handleSelectButton" src/wifi.cpp`

These are three back-to-back functions around line 507. Replace their bodies:

```cpp
void handleLeftButton() {
    spamchannel = channelDown(spamchannel, 14);
}

void handleRightButton() {
    spamchannel = channelUp(spamchannel, 14);
}

void handleSelectButton() {
    spam = toggleAttack(spam);
}
```

- [ ] **Step 8: Replace TLV block in BeaconSpammer `spammer()` in `src/wifi.cpp`**

Locate with: `grep -n "void spammer" src/wifi.cpp`

The function is around line 570. Replace the SSID-length cap + TLV-fill block while keeping the MAC randomization and the `esp_wifi_80211_tx` call. The refactored function:

```cpp
void spammer() {
    esp_wifi_set_channel(spamchannel, WIFI_SECOND_CHAN_NONE);

    for (int i = 10; i <= 21; i++) {
        packet[i] = random(256);
    }

    String randomSSID = ssidList[random(ssidCount)];
    int ssidLength = randomSSID.length();
    if (ssidLength > 32) ssidLength = 32;

    int packetSize = buildBeaconTlv(packet, sizeof(packet),
                                    randomSSID.c_str(), (uint8_t)ssidLength,
                                    spamchannel);
    if (packetSize < 0) return;

    esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false);

    delay(1);
}
```

- [ ] **Step 9: Verify hardware env still compiles**

```bash
pio run -e esp32_v1_hardware 2>&1 | tail -5
```

Expected: `[SUCCESS] Took N.N seconds`

- [ ] **Step 10: Run full native suite — verify all 83 tests pass**

```bash
pio test -e native_tests 2>&1 | tail -8
```

Expected:
```
test/test_wifi_frame   10 Tests 0 Failures 0 Ignored
test/test_wifi_scan    11 Tests 0 Failures 0 Ignored
test/test_wifi_beacon  13 Tests 0 Failures 0 Ignored
test/test_battery      22 Tests 0 Failures 0 Ignored
test/test_theme        27 Tests 0 Failures 0 Ignored
-----------------------
83 Tests 0 Failures 0 Ignored
OK
```

- [ ] **Step 11: Commit**

```bash
git add lib/logic/wifi_beacon_logic.h lib/logic/wifi_beacon_logic.cpp \
        test/test_wifi_beacon/test_main.cpp src/wifi.cpp
git commit -m "feat(logic): extract wifi_beacon_logic — channelDown/Up, toggleAttack, buildBeaconTlv + 13 tests"
```
