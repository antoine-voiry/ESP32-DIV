# WiFi Logic Extraction Design

**Goal:** Extract pure decision logic from `src/wifi.cpp` into three `lib/logic/` modules so it is unit-testable on host GCC without Arduino/ESP32 headers.

**Scope:** Tier 1 (frame predicates, channel wrap, network anomaly analysis, RSSI sort) + Tier 2 (channel navigation helpers, attack toggle, beacon TLV construction).

---

## Architecture

`src/wifi.cpp` keeps all hardware I/O (WiFi stack, TFT, PCF8574, EEPROM). The three new logic files contain only `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`, `<string.h>` — no Arduino or ESP-IDF headers. `src/wifi.cpp` includes the three headers and calls into them, forwarding results to hardware.

```
src/wifi.cpp  →  lib/logic/wifi_frame_logic.h   (frame predicates, channel wrap)
              →  lib/logic/wifi_scan_logic.h    (WifiApInfo, analyzeAp, compareApByRssi)
              →  lib/logic/wifi_beacon_logic.h  (channelDown/Up, toggleAttack, buildBeaconTlv)
```

No cross-dependencies between the three logic files.

---

## Module 1 — wifi_frame_logic

**Files:** `lib/logic/wifi_frame_logic.h`, `lib/logic/wifi_frame_logic.cpp`

**Purpose:** 802.11 frame type predicates used by PacketMonitor and DeauthDetect sniffer callbacks; channel boundary clamp used by PacketMonitor::setChannel.

### Signatures

```cpp
bool    isDeauthFrame(uint8_t frameType);
bool    isDisassocFrame(uint8_t frameType);
uint8_t wrapChannel(uint8_t ch, uint8_t minCh, uint8_t maxCh);
```

### Behaviour

- `isDeauthFrame`: returns `frameType == 0xC0`
- `isDisassocFrame`: returns `frameType == 0xA0`
- `wrapChannel`: if `ch < minCh || ch > maxCh` returns `minCh`, otherwise returns `ch`

### Call site changes in wifi.cpp

PacketMonitor `wifi_promiscuous()`:
```cpp
// before
if (type == WIFI_PKT_MGMT && (pkt->payload[0] == 0xA0 || pkt->payload[0] == 0xC0))
// after
if (type == WIFI_PKT_MGMT && (isDisassocFrame(pkt->payload[0]) || isDeauthFrame(pkt->payload[0])))
```

DeauthDetect `snifferCallback()`:
```cpp
// before
if (frameType == 0xC0)
// after
if (isDeauthFrame(frameType))
```

PacketMonitor `setChannel()`:
```cpp
// before
if (ch > MAX_CH || ch < 1) ch = 1;
// after
ch = wrapChannel(ch, 1, MAX_CH);
```

### Tests — test/test_wifi_frame/test_main.cpp

| Test | Input | Expected |
|------|-------|----------|
| `test_isDeauth_0xC0` | 0xC0 | true |
| `test_isDeauth_other` | 0xA0 | false |
| `test_isDeauth_zero` | 0x00 | false |
| `test_isDisassoc_0xA0` | 0xA0 | true |
| `test_isDisassoc_other` | 0xC0 | false |
| `test_wrap_in_range` | ch=6, min=1, max=13 | 6 |
| `test_wrap_above_max` | ch=14, min=1, max=13 | 1 |
| `test_wrap_below_min` | ch=0, min=1, max=13 | 1 |
| `test_wrap_at_min` | ch=1, min=1, max=13 | 1 |
| `test_wrap_at_max` | ch=13, min=1, max=13 | 13 |

---

## Module 2 — wifi_scan_logic

**Files:** `lib/logic/wifi_scan_logic.h`, `lib/logic/wifi_scan_logic.cpp`

**Purpose:** Network anomaly detection (evil twin, hidden SSID, non-standard channel, deauth attack) and RSSI sort comparator. Replaces `analyzeNetworks()` and `compare_ap()` in wifi.cpp.

### Data types

```cpp
struct WifiApInfo {
    char    ssid[33];   // null-terminated; ssid[0]=='\0' → hidden
    uint8_t bssid[6];
    int8_t  rssi;       // dBm
    uint8_t channel;
};

struct NetworkAnomaly {
    bool isHidden;       // ssid[0] == '\0'
    bool isEvilTwin;     // same SSID, different BSSID found in allAps[]
    bool isWeirdChannel; // channel > 13
    bool isUnderAttack;  // deauthCount > 5
};
```

### Signatures

```cpp
NetworkAnomaly analyzeAp(const WifiApInfo* ap,
                         const WifiApInfo* allAps, int apCount,
                         int deauthCount);

int compareApByRssi(const void* a, const void* b);
```

### Behaviour

`analyzeAp`:
- `isHidden`: `ap->ssid[0] == '\0'`
- `isEvilTwin`: for any `j` where `j`-th entry is not `ap` itself, `strcmp(ap->ssid, allAps[j].ssid) == 0 && memcmp(ap->bssid, allAps[j].bssid, 6) != 0` — preserves original behaviour including treating two hidden networks as each other's evil twin
- `isWeirdChannel`: `ap->channel > 13`
- `isUnderAttack`: `deauthCount > 5`

`compareApByRssi`: `((WifiApInfo*)b)->rssi - ((WifiApInfo*)a)->rssi` (descending, for qsort)

### Call site changes in wifi.cpp

DeauthDetect `analyzeNetworks(int n)` becomes a thin loop:
```cpp
void analyzeNetworks(int n) {
    for (int i = 0; i < n; i++) {
        checkButtonPress();
        if (exitMode) return;
        WifiApInfo ap;
        strncpy(ap.ssid, ssidLists[i].c_str(), 32); ap.ssid[32] = '\0';
        memcpy(ap.bssid, macList[i], 6);
        ap.channel = WiFi.channel(i);
        // build allAps[] inline or pass globals; see plan for exact approach
        NetworkAnomaly flags = analyzeAp(&ap, allAps, n, deauth[i]);
        // display flags via existing displayPrint() calls
    }
}
```

Deauther `compare_ap()` replaced by `compareApByRssi` directly in the `qsort` call.

### Tests — test/test_wifi_scan/test_main.cpp

| Test | Scenario | Expected flags |
|------|----------|----------------|
| `test_normal_ap` | SSID="Home", ch=6, deauth=0 | all false |
| `test_hidden_ssid` | ssid[0]='\0', ch=6, deauth=0 | isHidden=true |
| `test_evil_twin` | same SSID, different BSSID in allAps | isEvilTwin=true |
| `test_no_evil_twin_same_bssid` | same SSID, same BSSID | isEvilTwin=false |
| `test_no_evil_twin_single_ap` | one AP, no others | isEvilTwin=false |
| `test_weird_channel_14` | channel=14 | isWeirdChannel=true |
| `test_normal_channel_13` | channel=13 | isWeirdChannel=false |
| `test_under_attack_6` | deauthCount=6 | isUnderAttack=true |
| `test_not_under_attack_5` | deauthCount=5 | isUnderAttack=false |
| `test_compare_higher_rssi_first` | rssi=-30 vs -80 | -30 sorts first |
| `test_compare_equal_rssi` | rssi=-50 vs -50 | returns 0 |

---

## Module 3 — wifi_beacon_logic

**Files:** `lib/logic/wifi_beacon_logic.h`, `lib/logic/wifi_beacon_logic.cpp`

**Purpose:** Channel navigation helpers for BeaconSpammer UI, attack toggle, and beacon TLV frame construction extracted from `spammer()`.

### Signatures

```cpp
uint8_t channelDown(uint8_t ch, uint8_t maxCh);
uint8_t channelUp(uint8_t ch, uint8_t maxCh);
bool    toggleAttack(bool current);
int     buildBeaconTlv(uint8_t* packet, size_t bufLen,
                       const char* ssid, uint8_t ssidLen,
                       uint8_t channel);
```

### Behaviour

- `channelDown`: `(ch == 1) ? maxCh : ch - 1`
- `channelUp`: `(ch == maxCh) ? 1 : ch + 1`
- `toggleAttack`: `!current`
- `buildBeaconTlv`: caller provides `packet` pre-loaded with beacon template. Function writes:
  - `packet[37] = ssidLen`
  - `packet[38 .. 37+ssidLen] = ssid bytes`
  - `ratesOffset = 38 + ssidLen`; writes 10-byte Supported Rates TLV: tag=0x01, len=0x08, rates={0x82,0x84,0x8b,0x96,0x24,0x30,0x48,0x6c}
  - `dsOffset = ratesOffset + 10`; writes 3-byte DS Parameter TLV: tag=0x03, len=0x01, value=channel
  - Returns `dsOffset + 3` (total frame length)
  - If `bufLen < dsOffset + 3` returns -1 (overflow guard)
  - ssidLen must already be capped at 32 by caller; `buildBeaconTlv` does not re-cap

### Call site changes in wifi.cpp

BeaconSpammer `handleLeftButton()`, `handleRightButton()`, `handleSelectButton()` become one-liners:
```cpp
void handleLeftButton()   { spamchannel = channelDown(spamchannel, 14); }
void handleRightButton()  { spamchannel = channelUp(spamchannel, 14); }
void handleSelectButton() { spam = toggleAttack(spam); }
```

BeaconSpammer `spammer()` replaces its TLV block:
```cpp
if (ssidLength > 32) ssidLength = 32;
int packetSize = buildBeaconTlv(packet, sizeof(packet), randomSSID.c_str(), ssidLength, spamchannel);
if (packetSize < 0) return;  // overflow guard
esp_wifi_80211_tx(WIFI_IF_AP, packet, packetSize, false);
```

### Tests — test/test_wifi_beacon/test_main.cpp

| Test | Input | Expected |
|------|-------|----------|
| `test_channelDown_normal` | ch=6, max=14 | 5 |
| `test_channelDown_wraps` | ch=1, max=14 | 14 |
| `test_channelUp_normal` | ch=6, max=14 | 7 |
| `test_channelUp_wraps` | ch=14, max=14 | 1 |
| `test_toggleAttack_false` | false | true |
| `test_toggleAttack_true` | true | false |
| `test_buildBeaconTlv_size` | ssid="TEST", len=4, ch=6 | returns 51+4=55 |
| `test_buildBeaconTlv_ssid_written` | ssid="AB", len=2 | packet[38]='A', packet[39]='B' |
| `test_buildBeaconTlv_ssidlen_byte` | ssid="HI", len=2 | packet[37]==2 |
| `test_buildBeaconTlv_rates_tag` | any ssid len=3 | packet[41]==0x01 |
| `test_buildBeaconTlv_ds_tag` | ssid len=3 | packet[51]==0x03 |
| `test_buildBeaconTlv_ds_channel` | ch=11, ssid len=3 | packet[53]==11 |
| `test_buildBeaconTlv_overflow` | bufLen=10 (too small) | returns -1 |

---

## Testing strategy

Three independent test suites under `test/`. Each runs with `pio test -e native_tests -f test_wifi_frame` etc. All use only standard C types — no mocks needed (no HAL dependency in these modules).

Full suite gate: `pio test -e native_tests` must pass all 34 new tests (10 + 11 + 13) plus the existing 49, total ≥ 83 tests, 0 failures.

Branch coverage: every `if`/ternary branch in all three logic files is hit by at least one test.

---

## Constraints

- Zero Arduino/ESP-IDF headers in `lib/logic/` files
- `src/wifi.cpp` call sites must compile unchanged under `[env:esp32_v1_hardware]` after the refactor
- Do not alter the packet template bytes (lines 494-503 in wifi.cpp) — only the TLV-fill block moves
- `analyzeAp` must replicate original `analyzeNetworks` detection logic exactly, including the hidden-SSID evil-twin edge case
- Atomic commits: one commit per module (logic files + call site changes + tests together per module)
