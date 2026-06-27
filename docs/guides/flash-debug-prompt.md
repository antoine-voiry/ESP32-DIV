# Flash, Debug & Troubleshoot — Session Prompt

Paste this at the start of a new Claude Code session to restore full context.

---

## Prompt

```
ESP32-DIV V1 firmware — flash, validate, and troubleshoot session.

Context:
- Repo: /Users/antoine/esp32-div1/ESP32-DIV-clone, branch main
- Board: ESP32-WROOM-32 with PCF8574 I2C expander at 0x20 (buttons via I2C, NOT direct GPIO)
- All FATAL/PANIC software fixes are committed at HEAD (5f48cee). Working tree is clean.
- Build toolchain: Arduino Maker Workshop CLI, esp32:esp32@2.0.17, arduinoFFT 1.6.2, Mischianti PCF8574 2.4.0
- Flash port: /dev/cu.usbserial-0001
- ELF for panic decoding: build/ESP32-DIV-clone.ino.elf

CLI alias:
  CLI="/Users/antoine/.vscode/extensions/thelastoutpostworkshop.arduino-maker-workshop-1.1.5-darwin-arm64/arduino_cli/darwin/arm64/arduino-cli"

Flash command:
  "$CLI" upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32 .

Monitor command:
  "$CLI" monitor -p /dev/cu.usbserial-0001 --config baudrate=115200

Build command (if recompile needed):
  "$CLI" compile --fqbn esp32:esp32:esp32 \
    --board-options "FlashSize=16M,PartitionScheme=huge_app,CPUFreq=240,FlashMode=dio,FlashFreq=80,UploadSpeed=921600" \
    --warnings default \
    --build-property "compiler.c.elf.extra_flags=-Wl,-zmuldefs" \
    --output-dir build .

Task 11 validation checklist (DoD):
  1. Boot clean — no Guru Meditation, no Stack canary
  2. All 5 buttons navigate menus correctly
  3. BeaconSpammer: loop runs continuously, BTN_UP stops it
  4. PacketMonitor FFT: spectrum updates, SELECT exits within 200ms
  5. DeauthDetect x3: enter → 10s → Back (repeat); no panic; BeaconSpammer works after
  6. CaptivePortal + SubGHz: save data in each, switch, data intact (EEPROM fix)
  7. Navigation depth: main→WiFi→feature→Back x10 rapid, no crash

If a panic occurs, paste the full serial output here.
Use the /debug-panic skill to decode the backtrace.
Addr2line: xtensa-esp32-elf-addr2line -pfiaC -e build/ESP32-DIV-clone.ino.elf <addresses>
```

---

## Common Panic Signatures

| Serial output | Likely cause | File:line area |
|---------------|-------------|----------------|
| `Stack canary watchpoint triggered` | Stack overflow — recursion or deep call | `handleButtons()` callers |
| `Guru Meditation Error: Core 0 panic` | Check backtrace with addr2line | Any |
| `Mutex holder deleted` / `xQueueReceive` hang | Semaphore orphaned by vTaskDelete | `wifi.cpp` DeauthDetect cleanup |
| `Cache disabled but cached memory accessed` | IRAM attribute missing on ISR | ISR callbacks |
| Device reboots every ~5s silently | WDT reset — task starving scheduler | Busy-wait loops |
| Black screen after NRF24 feature | GPIO 4 clobbered | `bluetooth.cpp` cleanupNRF24 |
| SubGHz profile gone after CaptivePortal use | EEPROM collision | `eeprom_layout.h` (should be fixed) |

## Decode a Panic

1. Copy the full block from `Guru Meditation` through the last backtrace address.
2. Paste into the session.
3. Run:
   ```bash
   xtensa-esp32-elf-addr2line -pfiaC -e build/ESP32-DIV-clone.ino.elf 0x400XXXXX 0x400YYYYY ...
   ```
4. Or use the `/debug-panic` skill — it does this automatically.

## If Flash Fails

| Symptom | Fix |
|---------|-----|
| `Failed to connect to ESP32` | Hold BOOT button while upload starts, release after `Connecting...` |
| `Wrong port` | Check `ls /dev/cu.*` — port may be `cu.usbserial-XXXX` with different suffix |
| `A fatal error occurred: Timed out` | Lower upload speed: replace `UploadSpeed=921600` with `UploadSpeed=115200` |
| Upload succeeds but no serial output | Wrong baud rate — confirm 115200 |

## Fixes Applied (reference)

| Commit | Bug | What changed |
|--------|-----|-------------|
| `2e0df91` | FATAL-02 | `cleanupNRF24()` no longer resets GPIO 4 (TFT_BL) |
| `7ae9d77` | FATAL-03 | `eeprom_layout.h` — SubGHz moved to byte 1320, was 1280 |
| `fea831f` | FATAL-05 | BeaconSpammer BTN_UP exit: `!pcf.digitalRead()` |
| `7b58c94` | FATAL-06 | 16 recursive `handleButtons()` calls → `return` |
| `da6b08e` | FATAL-07 | FFT `while(micros...)` → `delayMicroseconds()` |
| `cf206e1` | FATAL-08 | DeauthDetect cooperative shutdown; `scanWiFiTask` stack 8192 |
| `c964036` | PANIC-01 | `drawStatusBar` uses passed voltage, no second ADC read |
| `c964036` | AMAT-08 | Removed duplicate `temprature_sens_read()` declaration |
