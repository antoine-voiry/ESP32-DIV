```
                         ██████████████████████
                    ████████████████████████████████
                 ████                              ████
              ████     ██████████████████████████     ████
            ███       ████                    ████       ███
          ███        ███    ████████████████    ███        ███
         ██         ██    ████████████████████    ██         ██
        ██          ██   ██████████████████████   ██          ██
        ██          ██  ████████████████████████  ██          ██
        ██          ██  ████████████████████████  ██          ██
        ██          ██   ██████████████████████   ██          ██
         ██         ██    ████████████████████    ██         ██
          ███        ███    ████████████████    ███        ███
            ███       ████                    ████       ███
              ████     ██████████████████████████     ████
                 ████                              ████
                    ████████████████████████████████
                         ██████████████████████

            I'm sorry, Dave. I'm afraid I can't do that.
```

# ESP32-DIV v0.1.0 — HAL9000 Edition

![ESP32](https://img.shields.io/badge/ESP32--WROOM--32U-blue?logo=espressif)
![Version](https://img.shields.io/badge/Version-0.1.0-red)
![License](https://img.shields.io/badge/License-Educational-orange)
![Status](https://img.shields.io/badge/Status-Alpha%2FUntested-yellow)

> Multi-radio offensive security platform with WiFi, BLE, SubGHz (CC1101), and 2.4GHz (NRF24L01+) capabilities.  
> HAL9000 Edition — HAL9000 visual identity port for the ESP32-DIV V1 hardware.

---

## V1 Board Owners

**This firmware targets original V1 ESP32-DIV boards** (ESP32-WROOM-32U).

CiferTech's official v1.5.0 firmware targets the newer V2 boards (ESP32-S3). If you have a V1 board, that firmware won't work for you.

| Your Board | Firmware |
|------------|----------|
| V1 (ESP32-WROOM-32U) | **This firmware** |
| V2 (ESP32-S3) | CiferTech v1.5.0 |

---

## What's in This Edition

### HAL9000 Visual Identity (v0.1.0)

A complete visual overhaul replacing the HaleHound skull motif with a HAL9000 eye theme:

- **HAL9000 Eye Icons** — 8 custom 16×16 navigation icons + 10-frame loading animation
- **HAL9000 Splash Screen** — Full-screen eye panel on startup (red on black)
- **HAL9000 Background** — 211×280 aperture panel on the home screen
- **Monochrome scanlines** — Home background replaced with horizontal scanline pattern
- **Branding** — "v0.1.0 - HAL9000 Edition" on splash and About screen
- **Temperature thresholds** — Recalibrated for ESP32 die temperature (idle ~50°C = green)

### Inherited from HaleHound Edition (v2.5.0)

Features carried forward from JMFH's port (see Credits):

| Feature | Description |
|---------|-------------|
| **Spectrum Analyzer** | 2.4GHz visualization with scrolling waterfall |
| **WLAN Jammer** | Targeted WiFi disruption via NRF24 |
| **Proto Kill** | Multi-protocol 2.4GHz disruption |
| **SubGHz Brute Force** | Automated code TX (Linear, CAME, Nice, Chamberlain, DoorHan, Gate TX) |
| **BLE Sniffer** | Passive Bluetooth packet capture |
| **Brightness Control** | Adjustable screen brightness |
| **Screen Timeout** | Configurable auto-sleep |
| **Full Touch Support** | Touch input on all menus |
| **SPI Bus Isolation** | Touch on HSPI, radios on VSPI — no more touch death after SubGHz |
| **Touch Calibration Constants** | Centralized in `Touchscreen.h` |
| **Phone Remote WebUI** | Browser-based remote control over WiFi AP |

---

## Hardware Requirements

| Component | Specification |
|-----------|---------------|
| **MCU** | ESP32-WROOM-32U |
| **Display** | 2.8" TFT LCD (240×320) ILI9341 |
| **Touch** | XPT2046 Touch Controller |
| **SubGHz** | CC1101 Transceiver (300–928 MHz) |
| **2.4GHz** | NRF24L01+ Transceiver |
| **Buttons** | PCF8574 I2C GPIO Expander (0x20) |
| **Storage** | SD Card + EEPROM |

---

## Features

### 2.4GHz Radio (NRF24L01+)
- **Channel Scanner** — Scan WiFi channels (2400–2484 MHz) with noise calibration
- **Spectrum Analyzer** — Signal visualization with scrolling waterfall
- **WLAN Jammer** — Targeted WiFi disruption
- **Proto Kill** — Multi-protocol 2.4GHz disruption

### SubGHz Radio (CC1101)
- **Replay Attack** — Capture and replay RF signals (garages, gates, car fobs)
- **Brute Force** — Automated code transmission
- **Jammer** — Broadband SubGHz jamming

### WiFi (ESP32)
- **Packet Monitor** — Real-time capture with channel hopping
- **Beacon Spammer** — Fake AP generation (Rickroll mode included)
- **Deauther** — Targeted deauthentication attacks
- **Deauth Detector** — Passive attack monitoring
- **WiFi Scanner** — Network enumeration
- **Captive Portal** — Evil twin credential harvesting

### Bluetooth (ESP32 BLE)
- **BLE Jammer** — Bluetooth Low Energy disruption
- **BLE Spoofer** — Device address cloning
- **Sour Apple** — iOS popup flooding
- **BLE Sniffer** — Passive packet capture
- **BLE Scanner** — Device discovery

### Tools & Utilities
- Serial Terminal
- OTA Firmware Update
- Brightness Control
- Screen Timeout
- Device Info (heap, CPU freq, flash, battery)

---

## Pin Configuration

### SPI Bus Architecture

| Bus | Function | MOSI | MISO | CLK | Devices |
|-----|----------|------|------|-----|---------|
| **VSPI** | Radios & Storage | 23 | 19 | 18 | NRF24, CC1101, SD Card |
| **HSPI** | Touch (Dedicated) | 32 | 35 | 25 | XPT2046 |

### Touch Controller (XPT2046) — HSPI
| Pin | GPIO |
|-----|------|
| IRQ | 34 |
| MOSI | 32 |
| MISO | 35 |
| CLK | 25 |
| CS | 33 |

### NRF24L01+
| Pin | GPIO |
|-----|------|
| CE | 4 |
| CSN | 5 |

### PCF8574 Buttons (I2C 0x20, SDA=21, SCL=22)
| Button | Expander Pin |
|--------|-------------|
| UP | 6 |
| DOWN | 3 |
| LEFT | 4 |
| RIGHT | 5 |
| SELECT | 7 |

> ⚠️ These are PCF8574 expander channels, NOT ESP32 GPIO numbers.

---

## CRITICAL: TFT_eSPI Library Configuration

**If you get a WHITE SCREEN after flashing, this is the fix.**

Copy `User_Setup.h` from this repo to your TFT_eSPI library folder:

**macOS / Linux:**
```bash
cp User_Setup.h ~/Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
```

**Windows:**
```batch
copy User_Setup.h %USERPROFILE%\Documents\Arduino\libraries\TFT_eSPI\User_Setup.h
```

### ESP32-DIV Pin Configuration (User_Setup.h)
```cpp
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   0
#define TFT_BL    4
#define USE_HSPI_PORT
```

---

## Build From Source

```bash
# Board: ESP32 Dev Module
# Partition: Huge APP (3MB No OTA/1MB SPIFFS)
# Flash: 4MB @ 240MHz

arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app .
```

### Required Libraries
- TFT_eSPI (ILI9341) — **Must configure User_Setup.h first**
- XPT2046_Touchscreen
- PCF8574
- RF24
- ELECHOUSE_CC1101_SRC_DRV
- RCSwitch
- arduinoFFT
- ESP32 BLE Arduino

---

## Credits

| | |
|---|---|
| **Original Firmware** | [CiferTech](https://github.com/cifertech) |
| **HaleHound Edition (v2.x base)** | JMFH — [github.com/JesseCHale/ESP32-DIV](https://github.com/JesseCHale/ESP32-DIV) |
| **HAL9000 Edition** | AVo |

This firmware is a derivative work. Original architecture by CiferTech; V1 hardware port and feature expansion by HaleHound (JMFH); HAL9000 visual identity and this fork by AVo.

---

<p align="center">
<b>For authorized security research only.</b>
</p>
