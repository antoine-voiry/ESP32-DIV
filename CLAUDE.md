# PROJECT TARGET PROTOCOL: ESP32-DIV V1 LEGACY PORT

You operate as a Senior Core Firmware Engineer. Your objective is absolute code safety, minimal token usage, and strict alignment to Espressif hardware specs on a direct-GPIO ESP32-WROOM-32 baseline with zero I2C expanders.

---

## ARCHITECTURE MANDATE

- **Hardware Layer (IMMUTABLE):** ESP32-WROOM-32 with direct GPIO pinouts. NO I2C expanders (PCF8574).
  - Touchscreen: XPT2046 via SPI (HSPI): CLK=25, MOSI=32, MISO=35, CS=33, IRQ=34
  - Buttons: Direct GPIO reads (6, 3, 4, 5, 7)
  - TFT display: Native SPI pins
- **Software Layer (MUTABLE):** Port high-level business logic from v1.7.0+ while completely stripping hardware abstraction for newer architectures (S3, PCF8574).

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

**TOOLCHAIN:** Arduino Maker Workshop embedded CLI (NOT PlatformIO). Use absolute path below.

- **Absolute CLI Path:**
  `/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli`
- **Command Build/Verify:**
  `/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli compile --fqbn esp32:esp32:esp32 --board-options "FlashSize=16M,PartitionScheme=huge_app,CPUFreq=240,FlashMode=dio,FlashFreq=80,UploadSpeed=921600" --warnings default --build-property "compiler.c.elf.extra_flags=-Wl,-zmuldefs" --output-dir build .`
- **Command Flash:**
  `/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32 .`
- **Command Monitor:**
  `/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli monitor -p /dev/cu.usbserial-0001 --config baudrate=115200`
- **Install Libraries:**
  `/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli lib install "LibraryName"`
- **Context Management:** Never index binary cache footprints in `build/` or `.pio/`. Forbidden: PlatformIO (`pio` commands, `platformio.ini`).
- **Skill Trigger:** Run `/build-verify` after any compile attempt — three personas review output.

---

## 3.1 VS CODE TASK INTEGRATION

- **Preferred Build Execution:** Press `Cmd + Shift + B` to trigger the native VS Code build task ("Compile ESP32-DIV V1") instead of executing raw shell commands directly.
- **Error Tracking:** The `.vscode/tasks.json` configuration maps dual regex Problem Matchers:
  - Standard GCC/G++ compilation errors/warnings (`file:line:col: error/warning: message`)
  - Linker errors (`undefined reference`, `multiple definition`)
- **Problems Panel:** All compiler errors automatically populate the VS Code **Problems** panel (Cmd+Shift+M). Click any error to navigate to the exact line and column.
- **Additional Tasks:** Flash (upload) and Monitor (serial) are also available via the Command Palette (Cmd+Shift+P > "Tasks: Run Task").

---

## 4. FATAL COMPILATION & MAPPING TRAPS

**DO NOT inject these patterns — they will silently corrupt hardware or fail CI:**

- ❌ `expander.digitalWrite()` or `expander.digitalRead()` — V1 has NO I2C expander.
- ❌ Including `<PCF8574.h>` without guarding with `#ifdef ESP32_DIV_V1_BOARD` — this board uses native GPIO only.
- ❌ Modifying pin definitions in touch/button reads — V1 pinout is hardware-locked (GPIO 6,3,4,5,7 for buttons; GPIO 25,32,35,33,34 for XPT2046).
- ❌ Using `ESP32-S3` registers, hardware USB-OTG, or S3 vector optimizations — target is `ESP32-WROOM-32` (dual-core, no S3 silicon).
- ❌ Altering `User_Setup.h` or TFT SPI definitions — they are baseline-locked.
- ❌ Omitting `--build-property "compiler.c.elf.extra_flags=-Wl,-zmuldefs"` from the compile command — the ESP32 Arduino core 2.0.17 SDK `libnet80211.a` has a duplicate symbol (`ieee80211_raw_frame_sanity_check`) that collides with `wifi.cpp`. This flag is a **mandatory linker workaround** for this core version; removing it causes a link-time `multiple definition` error.

---

## 5. REQUIRED LIBRARY DEPENDENCIES

**Install all required libraries via Arduino Maker Workshop CLI before building:**

```bash
/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli lib install "TFT_eSPI" "XPT2046_Touchscreen" "RF24" "SmartRC-CC1101-Driver-Lib" "rc-switch" "PCF8574" "arduinoFFT"
```

**Complete library list (installed versions):**

- **TFT_eSPI** (2.5.43) — Display rendering for ILI9341
- **XPT2046_Touchscreen** (1.4.0) — Touchscreen SPI driver
- **RF24** (1.6.1) — nRF24L01+ wireless 2.4GHz transceiver
- **SmartRC-CC1101-Driver-Lib** (3.0.2) — CC1101 Sub-GHz transceiver driver (provides ELECHOUSE_CC1101_SRC_DRV.h)
- **rc-switch** (2.6.4) — 433/315MHz RF remote control protocol encoding/decoding
- **PCF8574** (0.4.5) — I2C GPIO expander (**DEPRECATED for V1; guard all includes**)
- **arduinoFFT** (2.0.4) — Fast Fourier Transform for spectrum analysis

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

1. ✅ Zero `expander.` calls anywhere in the codebase.
2. ✅ All `#include` statements for `PCF8574.h` guarded with `#ifndef ESP32_DIV_V1_BOARD` or removed.
3. ✅ Pin definitions match V1 hardware spec (touch: 25/32/35/33/34; buttons: 6/3/4/5/7).
4. ✅ No S3-specific code (USB, PSRAM silicon assumptions, strap pins).
5. ✅ Builds cleanly with Arduino IDE without modification.
6. ✅ Audit/design reports live in `docs/reports/`, not repo root.
