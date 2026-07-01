# ESP32-DIV Bug List

---

## TECH-001 — Unit tests: no coverage due to hardware coupling

**Status:** Partially Fixed  
**Reported:** 2026-06-28  
**Partially fixed:** 2026-07-01  

**Summary:**  
All 8 source files (`theme.cpp`, `utils.cpp`, `webui.cpp`, `wifi.cpp`, `bluetooth.cpp`, `subghz.cpp`, `Touchscreen.cpp`, `main.cpp`) are tightly coupled to ESP32 hardware (TFT, SPI radios, BLE, WiFi, ADC, EEPROM). No native PlatformIO test environment exists yet, and no HAL (hardware abstraction layer) has been introduced.

**Progress (2026-07-01):**  
HAL abstraction (`lib/logic/`, `[env:native_tests]`) established in prior sessions. Coverage gaps closed this session: `computeCC1101Status` (2 tests), `validateThemeRaw` (4 tests), `buildBeaconTlv` ssidLen=0 (2 tests), `compareApByRssi` reversed-pair (1 test), `lookupOui` adversarial (4 tests), HAL9000 RGB565 pixel count (1 test). Total native suite: 153 tests. Remaining coverage gap: `isTouchLeft()` lives in `src/` (hardware-coupled, cannot run natively) — borderline exempt per CLAUDE.md §8.

---

## BUG-003 — Temperature icon always orange (wrong thresholds for die temp)

**Status:** Fixed  
**Severity:** Low  
**Reported:** 2026-06-30  
**Fixed:** 2026-06-30  

**Symptom:**  
Temperature icon in the status bar always displayed orange (`STATUS_WARN`), regardless of actual thermal state.

**Root cause:**  
`drawStatusBar()` used ambient-temperature thresholds (WARN ≥ 50°C, ERR ≥ 55°C) against `temprature_sens_read()`, which returns **ESP32 die temperature** — not ambient. ESP32 die temperature at idle is typically 47–53°C, which falls squarely in the WARN band.

**Fix:**  
Extracted threshold logic to `computeTempStatus(float tempC)` in `lib/logic/battery_logic.cpp`. Recalibrated thresholds for die temperature:
- OK: < 65°C (normal idle/light load)
- WARN: 65–80°C (elevated load)
- ERR: ≥ 80°C (heavy load / thermal stress)

**Regression tests added:**  
7 tests in `test/test_battery/test_main.cpp` — cover idle (53°C → OK), both boundary values (65°C, 80°C), and extremes.

---

## BUG-002 — WebUI buttons do nothing (WS commands not processed)

**Status:** Open  
**Severity:** High  
**Reported:** 2026-06-28  
**Affected firmware:** HEAD (post-WebUI branch)

**Symptom:**  
WebUI loads at `192.168.4.1`, tabs navigate, WS handshake returns `101 Switching Protocols`, but tapping any tool launch button has no effect — no TFT response, no `tool_started` event. STOP and STATUS buttons also unresponsive.

**Confirmed:**  
- HTTP `GET /` → 200 OK, 3111 bytes (gzip payload correct)  
- WS upgrade to `/ws` → 101 Switching Protocols  
- Properly masked WS text frames sent → no response from device

**Suspected cause (to investigate):**  
Unknown. WS connects but `onWsEvent` may not be firing, or `WebUIService::loop()` may not be processing `pendingCat`. Needs caveman Serial debug:  
1. Print in `onWsEvent` on `WS_EVT_DATA` to confirm callback fires  
2. Print `pendingCat` value in `WebUIService::loop()` to confirm it's being set  

**Debug approach already staged:**  
`webui.cpp onWsEvent` — add `Serial.print("[WS]type="); Serial.println(type);` on entry, and `Serial.print("[WS]pendingCat="); Serial.println(pendingCat);` after action parsing. Then monitor serial while sending a WS frame from the phone.

---

## BUG-010 — Phone remote: Start Server fails silently

**Status:** Open  
**Severity:** High  
**Reported:** 2026-06-30  

**Symptom:**  
Pressing UP on the Phone Remote screen to start the server has no visible effect — screen continues to show "Server: stopped" after the key press.

**Suspected causes (two independent issues):**  
1. **WiFi state conflict** — If any WiFi feature (scanner, deauther, beacon spammer) was used before navigating to Phone Remote, the WiFi stack may be in STA or scan mode. `WebUIService::setup()` starts an AP, which requires `WiFi.softAP()`. If the driver is still in STA mode from a prior feature, `softAP()` silently fails and `isActive()` returns false. `WebUIService::setup()` has no return value or error path, so failure is invisible.  
2. **`WebUIService::loop()` not called** — `webUIPhoneRemoteScreen()` runs its own blocking `while (!feature_exit_requested)` loop (utils.cpp). `WebUIService::loop()` in `main.cpp::loop()` is never reached while inside this blocking loop. Even if the AP starts, WebSocket message processing is stalled until the user exits the screen — meaning any browser-side button press goes unprocessed.

**Files:**  
`src/main.cpp` ~2299 (`webUIPhoneRemoteScreen` blocking loop), `src/webui.cpp` (`WebUIService::setup`)

---

## BUG-009 — Device info screen incomplete

**Status:** Open  
**Severity:** Low  
**Reported:** 2026-06-30  

**Symptom:**  
The Device Info screen (Tools → Device Info, `src/main.cpp` ~2220) shows only: Device name, Version, By, Original, Free Heap, CPU Freq, Flash size, Battery voltage. Missing information the user would expect for hardware diagnostics.

**Missing fields:**  
- WiFi MAC address (`WiFi.macAddress()`)  
- Chip ID / efuse MAC (`ESP.getEfuseMac()`)  
- SDK / IDF version (`ESP.getSdkVersion()`)  
- Uptime (`millis() / 1000`)  
- CC1101 detected (call `ELECHOUSE_cc1101.getCC1101()`)  
- NRF24 detected (call `radio.isChipConnected()`)  
- Flash used vs total (already has total; add `ESP.getSketchSize()`)  

**Files:**  
`src/main.cpp` ~2220 (`handleDeviceInfoScreen` or equivalent)

---

## BUG-008 — Proximity Wand: detected network disappears / rolls back

**Status:** Open  
**Severity:** Medium  
**Reported:** 2026-06-30  

**Symptom:**  
Proximity Wand briefly shows a detected network/signal, then immediately reverts to "NO SIGNAL" or the display resets.

**Root cause (suspected):**  
`ProximityWand::setup()` is called once per entry (`src/main.cpp` line 2639). It calls `initNRF24()` which does `SPI.end()` / `SPI.begin()` / `radio.begin()` and resets the channel to 2. `static` state variables `nrfHits`, `nrfMisses`, `nrfChannel`, `lastRenderedRssi` persist across `setup()` calls (no reset on entry), so stale RSSI from a previous session can flash briefly before the EWMA smoothing pulls it to floor. Additionally, `ProximityWand::setup()` is not idempotent — calling it while the NRF24 is mid-listen causes an SPI bus teardown that disrupts any in-progress capture, producing a momentary spike then immediate drop.

**Files:**  
`src/proximity_wand.cpp` (`initNRF24`, `static` hit/miss counters)

---

## BUG-007 — SD card icon always red

**Status:** Open  
**Severity:** High  
**Reported:** 2026-06-30  

**Root cause (two compounding issues):**

1. **GPIO 5 shared between SD_CS and NRF24 radio3 CSN** — `SD_CS_PIN = 5` (`src/wifi.cpp:3508`). NRF24 radio3 also uses GPIO 5 as its CSN. `wifi.cpp` documents this explicitly: `cleanupSD()` releases GPIO 5 before NRF24 operations (`src/wifi.cpp:15-29`). NRF24 is initialized at startup and keeps GPIO 5 as an SPI output. As a result, `SD.begin()` in `isSDCardAvailable()` cannot successfully claim GPIO 5 while NRF24 is active — which is essentially always. The SD card is structurally unavailable after boot on the current pin configuration.

2. **`SD.begin()` called on every status bar update** — `isSDCardAvailable()` (`src/utils.cpp:178`) calls `SD.begin()` unconditionally every 1000ms via `updateStatusBar()`. Even without the GPIO 5 conflict, `SD.begin()` returns `false` on an already-mounted card, so the icon would always be red regardless.

**Fix direction:**  
Short term: cache the mount result with a `static bool`; call `SD.begin(SD_CS_PIN)` once with explicit CS pin, reclaiming GPIO 5 from NRF24 first (or after NRF24 teardown). Long term: the SD and NRF24 pin sharing must be resolved at the hardware or SPI multiplexing level.

**Files:**  
`src/utils.cpp:177`, `src/wifi.cpp:3508` (`SD_CS_PIN`), `src/wifi.cpp:15` (`cleanupSD`)

---

## BUG-006 — CC1101 not detected / no presence check

**Status:** Fixed  
**Severity:** High  
**Reported:** 2026-06-30  
**Fixed:** 2026-07-01  

**Symptom:**  
SubGHz features produce no output; no indication on device that CC1101 is absent or failed to initialize.

**Root cause:**  
`CC1101Radio::init()` (`src/hal/CC1101Radio.h:8`) calls `ELECHOUSE_cc1101.Init()` which configures the chip but does not verify SPI communication. The ELECHOUSE library provides `ELECHOUSE_cc1101.getCC1101()` which performs a register readback to confirm chip presence — this is never called. If the CC1101 is not wired, powered, or SPI is blocked by another peripheral at init time, `Init()` silently returns and all subsequent operations produce garbage. `setup()` in `main.cpp` calls `gSubGhz->init()` with no return value or status check.

**Files:**  
`src/hal/CC1101Radio.h:8`, `src/main.cpp` (setup CC1101 init)

---

## BUG-005 — HAL9000 background bitmap redraw blocks for multiple seconds

**Status:** Fixed  
**Severity:** High  
**Reported:** 2026-06-30  
**Fixed:** 2026-07-01  

**Symptom:**  
Returning to the main menu (from any submenu, feature, or the About page) causes a multi-second freeze while the background redraws.

**Root cause:**  
`TFT_eSPI::drawBitmap()` renders a 1-bit XBM bitmap by calling `drawPixel()` once per set bit — no burst transfer. For the 211×280 HAL9000 background that is up to 59,080 individual SPI pixel writes. At 40 MHz SPI the transaction overhead alone puts this at 2–5 seconds. `displaySubmenu()` (`src/main.cpp:286`) always resets `menu_initialized = false`, so every return from any submenu triggers a full background redraw via `displayMenu()`.

**Impact:**  
- Root cause of BUG-004 (navigation appears frozen)
- Every menu transition back to main screen stalls for 2–5 seconds

**Fix direction:**  
Replace `drawBitmap()` with procedurally-drawn TFT primitives (`fillCircle`, `drawRoundRect`, etc.) — instant render, zero PROGMEM cost. Alternatively, pre-pack the bitmap as a 16-bit color array and use `pushImage()` for burst DMA transfer.

**Files:**  
`src/main.cpp:371` (`tft.drawBitmap(...hal9000_bg_bitmap...)`), `src/main.cpp:286` (`menu_initialized = false` in `displaySubmenu`)

---

## BUG-004 — Navigation buttons unresponsive after About → back

**Status:** Fixed (via BUG-005 fix)  
**Severity:** High  
**Reported:** 2026-06-30  

**Symptom:**  
After entering the About screen and pressing SELECT to return to the main menu, UP/DOWN navigation buttons appear unresponsive for several seconds.

**Root cause:**  
`handleAboutPage()` calls `displayMenu()` on back. `displayMenu()` checks `menu_initialized`, which was set to `false` by `displaySubmenu()` on menu entry. This triggers a full background redraw including `tft.drawBitmap(hal9000_bg_bitmap, 211, 280)` — a blocking 2–5 second SPI operation (see BUG-005). No button presses are processed during this blocking draw. The buttons are not broken; they are missed while the display is busy.

**Workaround:**  
Wait 5+ seconds after pressing SELECT from About — navigation will resume once the redraw completes.

**Dependency:**  
Fixing BUG-005 (slow bitmap) will resolve this symptom entirely.

**Files:**  
`src/main.cpp:3069` (`handleAboutPage` SELECT handler), `src/main.cpp:371` (bitmap redraw)

---

## BUG-001 — Left joystick unresponsive

**Status:** Workaround applied  
**Severity:** Medium  
**Reported:** 2026-06-28  
**Workaround:** 2026-07-01  
**Affected firmware:** HEAD (post-WebUI branch)

**Symptom:**  
Pressing LEFT on the 5-way joystick produces no navigation response on any menu. UP, DOWN, and SELECT are confirmed working. RIGHT untested.

**Hardware mapping:**  
BTN_LEFT = PCF8574 expander channel 4 (I2C 0x20, SDA=21, SCL=22).  
Native ESP32 GPIO 4 is NOT involved — the button is on the expander.

**Suspected cause:**  
Unknown. Either:
- PCF8574 pin P4 is physically damaged or has a cold solder joint on this unit
- The button switch itself is faulty
- Firmware regression (less likely — UP/DOWN/SELECT all read via the same `pcf.digitalRead()` path)

**To diagnose:**  
Check serial output at boot — setup() prints all 8 PCF8574 pin states:
```
Button 4: Released  ← should toggle to Pressed when LEFT held
```
If it always reads `Released` regardless of button state, the hardware is the issue.

**Workaround applied (2026-07-01):**  
`isTouchLeft()` added to `src/utils.cpp` — a tap in the leftmost 40px of the XPT2046 touchscreen acts as BTN_LEFT. All 5 `isButtonPressed(BTN_LEFT)` call sites in `main.cpp` replaced with `(isButtonPressed(BTN_LEFT) || isTouchLeft())`. Hardware root cause (PCF8574 P4 possibly damaged) still unresolved.

---

## BUG-011 — Beacon Spammer leaves WiFi in AP + promiscuous mode on exit

**Status:** Fixed  
**Severity:** High  
**Reported:** 2026-06-30  
**Fixed:** 2026-07-01  

**Symptom:**  
After exiting the Beacon Spammer feature, subsequent WiFi features (scanner, deauther, captive portal) behave erratically or fail to initialize. Phone Remote cannot start its AP.

**Root cause:**  
`beaconSpamSetup()` calls `esp_wifi_init()`, `esp_wifi_set_mode(WIFI_MODE_AP)`, `esp_wifi_start()`, and `esp_wifi_set_promiscuous(true)`. The exit path (SELECT press) breaks out of the feature loop and returns to submenu with no WiFi teardown. The WiFi stack remains running in AP + promiscuous mode for the rest of the session.

**Fix direction:**  
Add before returning from Beacon Spammer:
```cpp
esp_wifi_set_promiscuous(false);
esp_wifi_stop();
esp_wifi_deinit();
WiFi.mode(WIFI_OFF);
```

**Fix:**  
Added `BeaconSpammer::teardown()` in `src/wifi.cpp` — sets `spam = false`, then `esp_wifi_set_promiscuous(false)` → `esp_wifi_stop()` → `esp_wifi_deinit()` → `WiFi.mode(WIFI_OFF)`. Called on both exit paths (SELECT press and `feature_exit_requested`) in `src/main.cpp`. Declared in `include/wificonfig.h`.

**Unit tests:**  
Teardown body is entirely ESP32 WiFi API calls (hardware exception per CLAUDE.md §8 — no pure-logic branches to extract). No native unit tests added; validated by build + device.

---

## BUG-012 — Deauther leaves WiFi in AP mode on exit

**Status:** Fixed  
**Severity:** High  
**Reported:** 2026-06-30  
**Fixed:** 2026-07-01  

**Symptom:**  
Same class of issue as BUG-011. After exiting the Deauther, WiFi remains in AP mode, breaking subsequent WiFi feature initialization.

**Root cause:**  
`deautherSetup()` calls `esp_wifi_init()`, `esp_wifi_set_mode(WIFI_MODE_AP)`, `esp_wifi_start()`. The cleanup block inside `deautherSetup()` only runs when initialization *fails*, not on normal exit. The normal exit path (SELECT / back) has no WiFi teardown. `ap_list` heap buffer also leaked on re-entry.

**Fix:**  
Added `Deauther::teardown()` in `src/wifi.cpp` — sets `attack_running = false`, calls `freeBuffer((void**)&ap_list)` (extracted to `lib/logic/wifi_packet_logic.h`), then same WiFi deinit sequence as BUG-011. Called on both exit paths in `src/main.cpp`. Declared in `include/wificonfig.h`.

**Unit tests (2 added — `test/test_wifi_packet`):**  
`freeBuffer` extracted to `lib/logic/` (only extractable branch — uses `<stdlib.h>` only):
- `test_free_buffer_non_null_zeroes_pointer` — allocates buffer, calls freeBuffer, asserts pointer is null (catches double-free regression)
- `test_free_buffer_null_is_noop` — null input must not crash (catches missing null-guard)

---

## BUG-013 — BLE Scan resources not freed on exit

**Status:** Open  
**Severity:** Medium  
**Reported:** 2026-06-30  

**Symptom:**  
Rapidly entering and exiting BLE Scan causes increasing heap fragmentation. BLE stack remains active after exit, potentially conflicting with other BLE features (BLE Jammer, Sour Apple, BLE Sniffer).

**Root cause:**  
`bleScanSetup()` calls `BLEDevice::init("")` and `bleScan->setActiveScan(true)`. There is no corresponding `bleScan->stop()` / `BLEDevice::deinit()` on exit. By contrast, `blesnifferCleanup()` (`src/bluetooth.cpp:4752`) does properly clean up — BLE Scan is inconsistent with this pattern.

**Files:**  
`src/bluetooth.cpp` ~1524–1540 (bleScanSetup / bleScanLoop)

---

## BUG-014 — Feature re-entry on immediate SELECT after exit

**Status:** Open  
**Severity:** Medium  
**Reported:** 2026-06-30  

**Symptom:**  
After exiting a feature (e.g., Packet Monitor) via SELECT, pressing SELECT immediately again re-enters the same feature without navigating the submenu first.

**Root cause:**  
On feature exit, `current_submenu_index` is not changed and `in_sub_menu` is set to `true` with `displaySubmenu()` called. The next `handleButtons()` call dispatches back into the same submenu handler (e.g., `handleWiFiSubmenuButtons()`). Since `current_submenu_index` still equals the feature's index (e.g., 0 for Packet Monitor), the `if (current_submenu_index == 0)` block re-runs `ptmSetup()` immediately. The user must press UP or DOWN to change `current_submenu_index` before SELECT is safe. This pattern is systemic across all feature exit paths in all submenu handlers.

**Files:**  
`src/main.cpp` — all `handleXxxSubmenuButtons()` functions, feature exit paths

---

## BUG-015 — Dead code after return statements

**Status:** Open  
**Severity:** Low  
**Reported:** 2026-06-30  

**Symptom:**  
Cosmetic / code quality. Statements after `return` are never executed, indicating incomplete refactors.

**Examples:**  
- `src/main.cpp:466` — `is_main_menu = false;` after `return;`
- `src/main.cpp:710` — similar pattern
- `src/main.cpp:1157–1158` — similar pattern

Multiple occurrences throughout `handleButtons()` and submenu handlers.

**Files:**  
`src/main.cpp` — multiple locations in submenu and feature exit paths
