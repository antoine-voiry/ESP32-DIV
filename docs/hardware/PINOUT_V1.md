# ESP32-DIV V1 — Hardware Pinout Reference

**Target MCU:** ESP32-WROOM-32 (dual-core, 240 MHz, 4 MB flash)  
**Board revision:** V1 production (schematic commit 8c5bba2, April 2025)

> This document is the single authoritative source for GPIO assignments.
> Cross-verified against production schematics, BOM, and source code.
> Do not edit without updating the matching source-code constant.

---

## Schematics & Manufacturing Files

| File | Description |
|------|-------------|
| [schematics/ESP32DIV-main-board.jpg](schematics/ESP32DIV-main-board.jpg) | Main board schematic (production V1) |
| [schematics/ESP32DIV-shield.jpg](schematics/ESP32DIV-shield.jpg) | Shield schematic — NRF24 × 3, CC1101, IR |
| [bom/ESP32DIV-BOM.pdf](bom/ESP32DIV-BOM.pdf) | Main board Bill of Materials |
| [bom/ESP32DIV-SHIELD-BOM.pdf](bom/ESP32DIV-SHIELD-BOM.pdf) | Shield Bill of Materials |
| [gerber/ESP32DIV-Gerber.zip](gerber/ESP32DIV-Gerber.zip) | Main board Gerbers (ESP32-WROOM-32) |
| [gerber/ESP32DIV-SHIELD-Gerber.zip](gerber/ESP32DIV-SHIELD-Gerber.zip) | Shield Gerbers |

> **⚠️ Prototype schematic warning:** commit 21e1a9f (May 2024) contains an earlier
> prototype where buttons are wired to native ESP32 GPIOs — no PCF8574. That wiring
> is **incompatible with this firmware**. Do not use it for any hardware reference.

---

## GPIO Allocation Table

Every pin of the ESP32-WROOM-32 is accounted for below.  
`INPUT_ONLY` = pins 34–39 have no output driver on silicon.

| GPIO | Direction | Function | Peripheral | Bus | Source |
|------|-----------|----------|------------|-----|--------|
| 0 | OUT | TFT reset / boot strap | ILI9341 RST | HSPI | User_Setup.h |
| 1 | OUT | UART TX | CP2102 USB-serial | UART0 | hardware |
| 2 | OUT | TFT data/command | ILI9341 DC | HSPI | User_Setup.h |
| 3 | IN | UART RX | CP2102 USB-serial | UART0 | hardware |
| 4 | OUT | TFT backlight via Q3/8050 | ILI9341 BL + NRF24 radio3 CE | — | User_Setup.h / bluetooth.cpp:914 |
| 5 | OUT | SD card CS + NRF24 radio3 CSN | SD card / NRF24 U2 | VSPI | wifi.cpp / bluetooth.cpp:915 |
| 6 | — | **RESERVED — SPI flash CLK** | Internal flash | — | ESP32-WROOM-32 datasheet |
| 7 | — | **RESERVED — SPI flash D0** | Internal flash | — | ESP32-WROOM-32 datasheet |
| 8 | — | **RESERVED — SPI flash D1** | Internal flash | — | ESP32-WROOM-32 datasheet |
| 9 | — | **RESERVED — SPI flash D2** | Internal flash | — | ESP32-WROOM-32 datasheet |
| 10 | — | **RESERVED — SPI flash D3** | Internal flash | — | ESP32-WROOM-32 datasheet |
| 11 | — | **RESERVED — SPI flash CMD** | Internal flash | — | ESP32-WROOM-32 datasheet |
| 12 | IN | TFT MISO | ILI9341 | HSPI | User_Setup.h |
| 13 | OUT | TFT MOSI | ILI9341 | HSPI | User_Setup.h |
| 14 | OUT | TFT SCLK | ILI9341 | HSPI | User_Setup.h |
| 15 | OUT | TFT CS | ILI9341 | HSPI | User_Setup.h |
| 16 | IN/OUT | CC1101 GDO0 (TX data) + NRF24 radio1 CE | CC1101 / NRF24 U4 | — | subghz.cpp:138 / bluetooth.cpp:910 |
| 17 | OUT | NRF24 radio1 CSN | NRF24 U4 | VSPI | bluetooth.cpp:911 |
| 18 | OUT | SPI clock (radios + SD) | NRF24 × 3, CC1101, SD | VSPI | shield schematic |
| 19 | IN | SPI MISO (radios + SD) | NRF24 × 3, CC1101, SD | VSPI | shield schematic |
| 20 | — | Not available on ESP32-WROOM-32 | — | — | — |
| 21 | IN/OUT | I2C SDA | PCF8574 @ 0x20 | I2C | main.cpp / hardware |
| 22 | OUT | I2C SCL | PCF8574 @ 0x20 | I2C | main.cpp / hardware |
| 23 | OUT | SPI MOSI (radios + SD) | NRF24 × 3, CC1101, SD | VSPI | shield schematic |
| 24 | — | Not available on ESP32-WROOM-32 | — | — | — |
| 25 | OUT | XPT2046 SPI clock | Touchscreen | SW-SPI | Touchscreen.h |
| 26 | IN/OUT | CC1101 GDO2 (RX data) + NRF24 radio2 CE | CC1101 / NRF24 U3 | — | subghz.cpp:139 / bluetooth.cpp:912 |
| 27 | OUT | CC1101 CSN + NRF24 radio2 CSN | CC1101 / NRF24 U3 | VSPI | subghz.cpp:110 / bluetooth.cpp:913 |
| 28–31 | — | Not available on ESP32-WROOM-32 | — | — | — |
| 32 | OUT | XPT2046 MOSI | Touchscreen | SW-SPI | Touchscreen.h |
| 33 | OUT | XPT2046 CS | Touchscreen | SW-SPI | Touchscreen.h |
| 34 | INPUT_ONLY | XPT2046 IRQ | Touchscreen | — | Touchscreen.h |
| 35 | INPUT_ONLY | XPT2046 MISO | Touchscreen | SW-SPI | Touchscreen.h |
| 36 | INPUT_ONLY | (spare / SENSOR_VP) | — | — | — |
| 37 | INPUT_ONLY | Not available on ESP32-WROOM-32 | — | — | — |
| 38 | INPUT_ONLY | Not available on ESP32-WROOM-32 | — | — | — |
| 39 | INPUT_ONLY | (spare / SENSOR_VN) | — | — | — |

---

## SPI Bus Assignment

The ESP32-WROOM-32 has two hardware SPI controllers. This board uses both plus one software SPI.

| Bus | CLK | MOSI | MISO | Devices | CS pins |
|-----|-----|------|------|---------|---------|
| **HSPI** (hardware) | 14 | 13 | 12 | ILI9341 TFT | CS=15, DC=2 |
| **VSPI** (hardware) | 18 | 23 | 19 | NRF24 ×3, CC1101, SD card | CS=17, 27, 5 / CSN=5 |
| **SW-SPI** (software) | 25 | 32 | 35 | XPT2046 touchscreen | CS=33, IRQ=34 |

> `SPI.end()` is called when switching between NRF24 and CC1101 modes because they
> share VSPI. The HSPI (TFT) and SW-SPI (touch) are unaffected by these switches.

---

## I2C Bus

| GPIO | Role | Pull-up | Device | Address |
|------|------|---------|--------|---------|
| 21 | SDA | 4.7 kΩ to 3.3 V | PCF8574 | 0x20 |
| 22 | SCL | 4.7 kΩ to 3.3 V | PCF8574 | 0x20 |

Address 0x20 = A0=GND, A1=GND, A2=GND (all address lines pulled low on board).

---

## PCF8574 Expander Channel Map

**Critical:** these numbers are PCF8574 channel indices (P0–P7), **not ESP32 GPIO numbers**.  
The firmware reads them via `pcf.digitalRead(channel)` — never `digitalRead(channel)`.

| PCF8574 channel | Button | Firmware constant | Source |
|-----------------|--------|-------------------|--------|
| P0 | (spare) | — | — |
| P1 | (spare) | — | — |
| P2 | (spare) | — | — |
| P3 | DOWN | `BTN_DOWN = 3` | main.cpp:33 |
| P4 | LEFT | `BTN_LEFT = 4` | main.cpp:34 |
| P5 | RIGHT | `BTN_RIGHT = 5` | main.cpp:35 |
| P6 | UP | `BTN_UP = 6` | main.cpp:32 |
| P7 | SELECT | `BTN_SELECT = 7` | main.cpp:36 |

Buttons are active-LOW: pressed = `pcf.digitalRead()` returns 0.

---

## Radio Assignments

### NRF24L01+ (×3 on shield)

All three share VSPI (CLK=18, MOSI=23, MISO=19). Each has its own CE and CSN.

| Instance | CE GPIO | CSN GPIO | Shield module | Firmware |
|----------|---------|---------|---------------|---------|
| radio1 | 16 | 17 | U4 | bluetooth.cpp:910–911 |
| radio2 | 26 | 27 | U3 | bluetooth.cpp:912–913 |
| radio3 | 4 | 5 | U2 | bluetooth.cpp:914–915 |

### CC1101 Sub-GHz (on shield)

| Signal | GPIO | Direction | Notes |
|--------|------|-----------|-------|
| CSN | 27 | OUT | Shared with NRF24 radio2 CSN |
| GDO0 | 16 | OUT (ESP32→CC1101) | Async TX data; shared with NRF24 radio1 CE |
| GDO2 | 26 | IN (CC1101→ESP32) | Async RX data; shared with NRF24 radio2 CE |
| SCK | 18 | OUT | VSPI shared |
| MOSI | 23 | OUT | VSPI shared |
| MISO | 19 | IN | VSPI shared |

---

## Shared-Pin Conflict Table

These pins are physically connected to two peripherals simultaneously. Only one may be active at a time. Software enforces ordering via `cleanupNRF24()` (bluetooth.cpp) and `SPI.end()` / `pinMode(x, INPUT)` (subghz.cpp).

| GPIO | Peripheral A | Peripheral B | Risk if both active | Mitigation |
|------|-------------|-------------|---------------------|-----------|
| 4 | TFT backlight PWM (ILI9341 BL) | NRF24 radio3 CE (U2) | Backlight PWM toggles CE → spurious NRF24 transmissions | Backlight held static before radio use |
| 5 | SD card CS | NRF24 radio3 CSN (U2) | SPI bus collision | `cleanupNRF24()` before `SD.begin()` |
| 16 | CC1101 GDO0 (TX data OUT) | NRF24 radio1 CE (U4) | CE driven by CC1101 data stream | `pinMode(16, INPUT)` releases pin for NRF24 init |
| 26 | CC1101 GDO2 (RX data IN) | NRF24 radio2 CE (U3) | Same as above | `pinMode(26, INPUT)` releases pin |
| 27 | CC1101 CSN | NRF24 radio2 CSN (U3) | Double-select on VSPI bus | `digitalWrite(27, HIGH)` before mode switch |

---

## ⚠️ DANGER ZONE — Never Reconfigure These GPIOs

| GPIO | Hardware reason | Failure mode if driven |
|------|----------------|----------------------|
| 6 | ESP32-WROOM-32 internal SPI flash CLK | `TG1WDT_SYS_RESET` + cache-disabled panic, unrecoverable boot loop |
| 7 | Internal flash D0 | Same |
| 8 | Internal flash D1 | Same |
| 9 | Internal flash D2 | Same |
| 10 | Internal flash D3 | Same |
| 11 | Internal flash CMD | Same |
| 3 | UART0 RX — CP2102 bridge | Breaks serial monitor; DTR-based auto-reset stops working |
| 1 | UART0 TX — CP2102 bridge | Same |

> **The numbers 3, 4, 5, 6, 7 appear in `BTN_DOWN`, `BTN_LEFT`, `BTN_RIGHT`,
> `BTN_UP` defines but they refer to PCF8574 expander channels — not ESP32 GPIOs.**
> Confusing these is the single highest-risk mistake in this codebase.

---

## Known Naming Inconsistency in Source Code

In [src/subghz.cpp](../../src/subghz.cpp) lines 138–139:

```cpp
#define RX_PIN 16   // drives CC1101 GDO0 — actually the TX data output
#define TX_PIN 26   // reads CC1101 GDO2  — actually the RX data input
```

The variable names are **backwards** relative to signal direction. The comment on line 580
confirms this: `"RX_PIN=16=GDO0 is TX data line"`.

From the ESP32's perspective:
- GPIO 16 is **OUTPUT** (ESP32 sends data to CC1101 via GDO0)
- GPIO 26 is **INPUT** (ESP32 receives data from CC1101 via GDO2)

Do not trust `RX_PIN`/`TX_PIN` variable names alone — always read the `pinMode()` call
or the comment to determine actual direction.

---

## Shield Connector Pinout (N-Header 10×2, P2)

The shield plugs into the main board via a 20-pin header exposing these VSPI and control lines.

| Pin | Signal | GPIO |
|-----|--------|------|
| 1 | IO16 | 16 — NRF24 U4 CE / CC1101 GDO0 |
| 2 | IO18 | 18 — VSPI CLK |
| 3 | IO17 | 17 — NRF24 U4 CSN |
| 4 | IO23 | 23 — VSPI MOSI |
| 5 | IO26 | 26 — NRF24 U3 CE / CC1101 GDO2 |
| 6 | IO19 | 19 — VSPI MISO |
| 7 | IO27 | 27 — NRF24 U3 CSN / CC1101 CSN |
| 8 | TX_0 | 1 — UART TX (do not drive) |
| 9 | IO4 / Signal_R | 4 — NRF24 U2 CE / IR LED R |
| 10 | RX_0 | 3 — UART RX (do not drive) |
| 11 | IO5 / Signal_S | 5 — NRF24 U2 CSN / IR LED S |
| 12 | — | spare |
| 13 | — | spare |
| 14 | SCL | 22 — I2C SCL |
| 15–16 | GND | — |
| 17–18 | — | spare |
| 19–20 | +5 V / 3.3 V | power rails |

---

## Power Rails

| Rail | Source | Regulator | Notes |
|------|--------|-----------|-------|
| 3.3 V | Li-Ion battery or USB | LF33 (LDO) | Main logic rail |
| 5 V | USB VBUS | — | CP2102 supply, not always present |
| Battery | Li-Ion 1S | TP4056 charger + protection | Charged via USB-C TYPE-C-31-M-12 |

Backlight transistor Q3 (8050 NPN) base driven via R26 (1 kΩ) from GPIO 4.

---

*Cross-verified against: `User_Setup.h`, `include/Touchscreen.h`, `include/utils.h`,
`src/main.cpp:32–36`, `src/bluetooth.cpp:910–915`, `src/subghz.cpp:110,138–139`,
`src/wifi.cpp` (SD_CS_PIN), production schematic commit 8c5bba2, BOM commit fe1eb89.*
