# PROJECT TARGET PROTOCOL: ESP32-DIV V1 LEGACY PORT

You operate as a Senior Core Firmware Engineer. Your objective is absolute code safety, minimal token usage, and strict alignment to Espressif hardware specs on an ESP32-WROOM-32 baseline whose navigation buttons are driven through a PCF8574 I2C expander (GPIO starvation forces buttons off the native MCU pins).

---

## ARCHITECTURE MANDATE

- **Hardware Layer (IMMUTABLE):** ESP32-WROOM-32. Buttons are wired to a **PCF8574 I2C expander** (NOT native GPIO) due to severe pin starvation — the TFT, SD card, CC1101 (Sub-GHz) and NRF24L01 (2.4GHz) consume nearly all SPI/GPIO lines.
  - Touchscreen: XPT2046 via SPI (HSPI): CLK=25, MOSI=32, MISO=35, CS=33, IRQ=34
  - Buttons: **PCF8574 expander channels** 6/3/4/5/7 (UP/DOWN/LEFT/RIGHT/SELECT), read via `pcf.digitalRead()` on the default I2C bus (SDA=21, SCL=22). These numbers are expander P0–P7 lines, **never** ESP32 GPIOs.
  - TFT display: Native SPI pins
  - Radios: CC1101 (Antenna 1) + NRF24L01+ (Antenna 2) on the shared SPI bus, each with its own CS (and CE/GDO lines).
- **Software Layer (MUTABLE):** Port high-level business logic from v1.7.0+. Preserve the PCF8574 button abstraction; only strip hardware support for *newer* architectures (ESP32-S3).

---

## 1. FIRMWARE SYSTEM PERSONA & STYLE

- **Tone:** Technical, direct, data-driven. Zero conversational padding.
- **Syntax rules:** Enforce strict C/C++ style rules (ESP-IDF / PlatformIO standard conventions).
- **Variable rules:** Local pointers must be audited for NULL values post-initialization. All ISR context mutations must use `volatile`.

---

## 2. LOCAL FIRMWARE DEVELOPMENT SKILLS

### [SKILL: Audit FreeRTOS Context]

*When tasked with code review, file optimizations, or concurrency debugging:*

- Identify and eliminate any busy-waiting hardware loops (`while(cond);`).
- Enforce non-blocking scheduling execution using `vTaskDelay(pdMS_TO_TICKS(ms))`.
- Audit task stack allocations (`xTaskCreatePinnedToCore`) for potential stack overflow space constraints.
- Verify cross-core synchronization structures (`portMUX_TYPE`) are isolated to fast-path executions.

### [SKILL: Parse Hardware Panic Logs]

*When a backtrace, core dump, or hex memory address string is provided:*

- **Stop immediate execution.** Do not guess code coordinates.
- Output the clean host cross-compiler syntax mapping to find the faulty line:
  `xtensa-esp32-elf-addr2line -pfiaC -e build/*.elf <addresses>`
- Scan log streams for `Cache disabled but cached memory accessed` or `Stack canary watchpoint triggered`.

### [SKILL: Optimize Local Memory Footprint]

*When drafting changes to structural definitions, drivers, or partition schemes:*

- Enforce fixed-point math variables instead of float/double configurations where possible.
- Wrap all peripheral init declarations with `ESP_ERROR_CHECK()` assertions.
- Tag fast-execution critical routines with the explicit `IRAM_ATTR` qualifier.

---

## 3. DIRECT ACTION SHORTCUTS

**TOOLCHAIN:** PlatformIO 6.x (CLI: `platformio` or `pio`).

- **Build/Verify:**
  `platformio run`
- **Flash:**
  `platformio run --target upload`
- **Monitor:**
  `platformio device monitor`
- **Install/update deps:** Edit `lib_deps` in `platformio.ini` — PlatformIO resolves on next build.
- **Skill Trigger:** Run `/build-verify` after any compile attempt — three personas review output.

> Library versions are pinned in `platformio.ini` `lib_deps`. Do NOT run `pio update` without verifying version compatibility first.

---

## 3.1 VS CODE TASK INTEGRATION

- **Preferred Build Execution:** Press `Cmd + Shift + B` to trigger the PlatformIO build task via `.vscode/tasks.json`.
- **Additional Tasks:** Flash (upload) and Monitor (serial) are also available via the Command Palette (Cmd+Shift+P > "Tasks: Run Task").
- **Problems Panel:** Compiler errors automatically populate the VS Code **Problems** panel (Cmd+Shift+M) for quick navigation to error locations.

---

## 4. FATAL COMPILATION & MAPPING TRAPS

**DO NOT inject these patterns — they will silently corrupt hardware or fail CI:**

- ❌ **Assigning `pinMode()`/`digitalRead()` directly to button numbers 6/3/4/5/7 on the ESP32** — these are PCF8574 expander channels. Native GPIO 6/7 are SPI-flash lines and GPIO 3 is UART0 RX; driving them reconfigures the flash/UART interface and triggers an immediate `TG1WDT_SYS_RESET` cache-panic boot loop. Always read buttons via `pcf.digitalRead()`.
- ❌ Removing `<PCF8574.h>` or the `PCF8574 pcf(0x20)` object — buttons depend on it. Read it on the default I2C bus (SDA=21, SCL=22).
- ❌ Modifying pin definitions in touch/button reads — pinout is hardware-locked (PCF8574 channels 6,3,4,5,7 for buttons; GPIO 25,32,35,33,34 for XPT2046).
- ❌ Using `ESP32-S3` registers, hardware USB-OTG, or S3 vector optimizations — target is `ESP32-WROOM-32` (dual-core, no S3 silicon).
- ❌ Altering `User_Setup.h` or TFT SPI definitions — they are baseline-locked.
- ❌ Omitting the `-Wl,-zmuldefs` linker flag from the build — the ESP32 Arduino core 2.0.17 SDK `libnet80211.a` has a duplicate symbol (`ieee80211_raw_frame_sanity_check`) that collides with `wifi.cpp`. This flag is **mandatory** in `platformio.ini` `build_flags`; removing it causes a link-time `multiple definition` error.

---

## 5. REQUIRED LIBRARY DEPENDENCIES

**Library dependencies are managed via `platformio.ini` `lib_deps`.**

PlatformIO automatically resolves and installs all required libraries on the next build.

**Complete library list (pinned versions):**

- **TFT_eSPI** (2.5.43) — Display rendering for ILI9341
- **XPT2046_Touchscreen** (1.4.0) — Touchscreen SPI driver
- **RF24** (1.6.1) — nRF24L01+ wireless 2.4GHz transceiver
- **SmartRC-CC1101-Driver-Lib** (3.0.2) — CC1101 Sub-GHz transceiver driver (provides ELECHOUSE_CC1101_SRC_DRV.h)
- **rc-switch** (2.6.4) — 433/315MHz RF remote control protocol encoding/decoding
- **PCF8574** (2.3.7) — I2C GPIO expander (**REQUIRED for V1: drives the navigation buttons at 0x20 on SDA=21/SCL=22**) — PlatformIO registry publishes 2.x; API identical to 0.4.x
- **arduinoFFT** (1.6.2) — Fast Fourier Transform for spectrum analysis — pinned to 1.x; arduinoFFT 2.x renamed the class to `ArduinoFFT<T>` breaking this codebase

**⚠️ CC1101 Compatibility Note:**
The codebase includes `<ELECHOUSE_CC1101_ESP32DIV.h>` which is provided by a compatibility wrapper in the project root ([ELECHOUSE_CC1101_ESP32DIV.h](ELECHOUSE_CC1101_ESP32DIV.h)). This wrapper redirects to SmartRC-CC1101-Driver-Lib's `ELECHOUSE_CC1101_SRC_DRV.h`, maintaining backward compatibility with legacy code while using the modern, registry-available library.

---

## 6. DOCS FOLDER CONVENTION

All non-source artifacts live under `docs/` and are committed to the repo:

| Path | Contents |
|------|----------|
| `docs/reports/` | Audit reports (`audit-report-YYYYMMDD-*.md`) and design reports (`design-report-YYYYMMDD.md`) |
| `docs/superpowers/plans/` | Implementation plans generated by the writing-plans skill |
| `docs/superpowers/specs/` | Design specs generated by the brainstorming skill |

**Naming convention:** `audit-report-YYYYMMDD-HHMM.md`, `design-report-YYYYMMDD.md`

> `.gitignore` excludes `audit-report-*.md` and `design-report-*.md` at repo root — always place them in `docs/reports/` before committing.

---

## 7. BEFORE COMMITTING CHANGES

**Every pull request MUST satisfy:**

1. ✅ Buttons read via `pcf.digitalRead()` — zero direct `pinMode()`/`digitalRead()` on channels 6/3/4/5/7. `<PCF8574.h>` and the `PCF8574 pcf(0x20)` object are present and used.
2. ✅ No native-GPIO access to flash pins (6/7) or UART0 RX (3).
3. ✅ Pin definitions match V1 hardware spec (touch: GPIO 25/32/35/33/34; buttons: PCF8574 channels 6/3/4/5/7 via I2C SDA=21/SCL=22).
4. ✅ No S3-specific code (USB, PSRAM silicon assumptions, strap pins).
5. ✅ Builds cleanly with Arduino IDE without modification.
6. ✅ Audit/design reports live in `docs/reports/`, not repo root.
7. ✅ Unit test **branch coverage ≥ 90%** for any pure-logic code added or modified. No dead code. (Line coverage is not the metric — every reachable branch must be exercised. See TECH-001 in `docs/bugs/bug-list.md` for current coverage debt.)
8. ✅ **Any function with a non-trivial conditional (`if`/`else`, `switch`, ternary with side-effects) lives in `lib/logic/`**, not in `src/`. `src/` files are hardware I/O wrappers only — they call into `lib/logic/` and forward results to hardware. Logic that cannot be expressed without Arduino/ESP32 hardware headers is the only exception. Enforcement: if a new function in `src/` has a branch, ask "can this be extracted?" — if yes, it must be.
