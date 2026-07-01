// src/proximity_wand.cpp
// Hardware wrapper for the Proximity Wand module.
// Implements non-blocking scheduler: radio poll (80 ms) + TFT render (120 ms).
// All conditional logic lives in lib/logic/proximity_logic.h.
// SPI switching follows the bus-release protocol established in bluetooth.cpp
// and subghz.cpp; DEASSERT_SPI_BUS() adds an explicit CS-line guard layer.

#include "proximity_wand.h"
#include "bleconfig.h"    // cleanupNRF24()
#include "subconfig.h"    // cleanupSubGHz(), ELECHOUSE_cc1101
#include "shared.h"       // BG_BASE, STATUS_ERR, etc.
#include <SPI.h>
#include <RF24.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern PCF8574  pcf;

// Button channel numbers are PCF8574 expander lines, NOT ESP32 GPIOs.
// See docs/hardware/PINOUT_V1.md — PCF8574 channel map.
static constexpr uint8_t BTN_UP_CH     = 6;
static constexpr uint8_t BTN_DOWN_CH   = 3;
static constexpr uint8_t BTN_SELECT_CH = 7;

// ── NRF24 instance for 2.4 GHz proximity scan ────────────────────────────────
// radio1 wiring per PINOUT_V1.md: CE = GPIO 16, CSN = GPIO 17.
// The 16 000 000 argument is the SPI clock speed in Hz.
static RF24 proximityRadio(16, 17, 16000000);

// ── Scan mode ─────────────────────────────────────────────────────────────────
enum ScanMode : uint8_t { MODE_SUBGHZ = 0, MODE_2G4 = 1 };
static ScanMode scanMode = MODE_SUBGHZ;

// ── Shared scan state (written by radio task, read by render task) ────────────
static volatile int8_t  gRssi          = -100;
static volatile int16_t gRssiSmoothed  = -100;  // EWMA state, plain dBm units
static char             gVendor[16]    = "";
static bool             gThreat        = false;
static uint8_t          gMac3[3]       = {0, 0, 0};

// ── Non-blocking timer state ──────────────────────────────────────────────────
static unsigned long lastRadioMs  = 0;
static unsigned long lastRenderMs = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

static void initCC1101() {
    DEASSERT_SPI_BUS();
    SPI.end();
    delayMicroseconds(500);

    // GDO0 (GPIO 16) = OUTPUT for TX; GDO2 (GPIO 26) = INPUT for RX.
    // setGDO() calls GDO_Set() which configures the pinMode internally.
    ELECHOUSE_cc1101.setGDO(16, 26);
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setCCMode(0);       // raw/RCSwitch async mode
    ELECHOUSE_cc1101.setModulation(2);   // ASK/OOK
    ELECHOUSE_cc1101.setMHZ(433.92);     // default Sub-GHz frequency
    ELECHOUSE_cc1101.setRxBW(812.50);
    ELECHOUSE_cc1101.SetRx();
    Serial.println(F("[ProxWand] CC1101 ready (433.92 MHz, ASK)"));
}

static void initNRF24() {
    DEASSERT_SPI_BUS();
    SPI.end();
    delayMicroseconds(500);
    SPI.begin();

    if (!proximityRadio.begin()) {
        Serial.println(F("[ProxWand] NRF24 init failed — check wiring"));
        return;
    }
    // Listen on BLE advertising channel 37 (NRF24 RF channel 2).
    // Access address 0x8E89BED6 is the BLE advertising preamble reversed,
    // allowing the NRF24 to capture BLE advertisement bursts.
    proximityRadio.setAutoAck(false);
    proximityRadio.setDataRate(RF24_2MBPS);   // BLE uses 1 Mbps; 2 Mbps widens capture
    proximityRadio.setPayloadSize(32);
    proximityRadio.openReadingPipe(0, 0x8E89BED6LL);
    proximityRadio.setChannel(2);
    proximityRadio.startListening();
    Serial.println(F("[ProxWand] NRF24 BLE-sniff ready (ch 2 / 2402 MHz)"));
}

// ── CC1101 poll: update gRssi with EWMA smoothing ────────────────────────────
static void pollCC1101() {
    int8_t raw = static_cast<int8_t>(ELECHOUSE_cc1101.getRssi());
    gRssi = ewmaRssi(gRssiSmoothed, raw);
    // Sub-GHz signals don't carry OUI; clear the vendor label.
    gVendor[0] = '\0';
    gThreat    = false;
}

// ── NRF24 poll: capture one packet, extract OUI, synthesise dBm proxy ────────
// The NRF24L01+ has no per-packet RSSI register. Presence of any packet on
// the BLE advertising channel is mapped to an estimated proximity dBm based
// on receive success rate over the last poll window.
static uint8_t  nrfHits     = 0;  // packets captured this window
static uint8_t  nrfMisses   = 0;
static uint8_t  nrfChannel  = 0;  // index into BLE_CHANNELS[]

static const uint8_t BLE_CHANNELS[]   = {2, 26, 80};
static const uint8_t N_BLE_CHANNELS   = 3;

static void pollNRF24() {
    // Hop to next BLE channel each poll cycle.
    nrfChannel = (nrfChannel + 1) % N_BLE_CHANNELS;
    proximityRadio.stopListening();
    proximityRadio.setChannel(BLE_CHANNELS[nrfChannel]);
    proximityRadio.startListening();

    nrfHits = 0;
    uint8_t payload[32];

    // Drain available packets without blocking.
    while (proximityRadio.available()) {
        proximityRadio.read(payload, 32);
        ++nrfHits;

        // BLE LL PDU layout after access address:
        // [0] PDU header byte 0  [1] PDU header byte 1 (length)
        // [2..7] Advertiser MAC address (little-endian)
        // Bytes 4-6 are the OUI portion reversed: payload[4],payload[5],payload[6].
        if (nrfHits == 1 && payload[1] <= 30) {
            gMac3[0] = payload[4];
            gMac3[1] = payload[5];
            gMac3[2] = payload[6];
            lookupOui(gMac3, OUI_TABLE, OUI_TABLE_LEN, gVendor, &gThreat);
        }
    }

    int8_t proxy = hitsToRssiProxy(nrfHits);
    gRssi = ewmaRssi(gRssiSmoothed, proxy);
}

// ─────────────────────────────────────────────────────────────────────────────
// TFT Render
// ─────────────────────────────────────────────────────────────────────────────
// Screen layout (240 × 320):
//   Y   0 –  34 : header — mode label + frequency
//   Y  50 – 130 : large RSSI value (Font 7, size 2)
//   Y 145 – 170 : vendor / OUI label (Font 4)
//   Y 185 – 220 : proximity bar graph (35 px tall, full width)
//   Y 228 – 248 : zone label (Font 2)
//   Y 310 – 320 : button hint strip

static int8_t  lastRenderedRssi = -127;  // force first full redraw
static uint8_t lastZone         = 0xFF;

static void renderHeader() {
    tft.fillRect(0, 0, 240, 34, PW_BG);
    tft.setTextColor(PW_AMBER, PW_BG);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setCursor(4, 4);
    tft.print(scanMode == MODE_SUBGHZ ? F("SUB-GHz 433.92 MHz") : F("2.4 GHz  BLE SCAN"));
    tft.setCursor(4, 20);
    tft.setTextColor(PW_TEXT, PW_BG);
    tft.print(F("PROXIMITY WAND"));
}

static void renderRssi(int8_t rssi) {
    // Erase previous number area.
    tft.fillRect(0, 48, 240, 90, PW_BG);

    RssiZone zone = classifyRssi(rssi);
    uint16_t col  = zoneColor(zone);

    tft.setTextColor(col, PW_BG);
    tft.setTextFont(7);   // 48 px 7-segment style
    tft.setTextSize(2);   // ~96 px — readable at arm's length
    tft.setTextDatum(MC_DATUM);  // middle-centre alignment
    tft.drawString(String(rssi), 120, 95);

    // 'dBm' unit — smaller font, right-aligned below the number
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(PW_TEXT, PW_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(F("dBm"), 120, 140);
}

static void renderVendor() {
    tft.fillRect(0, 148, 240, 28, PW_BG);
    tft.setTextFont(4);   // 26 px — large enough to read without glasses
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);

    if (gVendor[0] == '\0') {
        tft.setTextColor(PW_DIM, PW_BG);
        tft.drawString(F("---"), 120, 162);
    } else {
        uint16_t col = gThreat ? PW_THREAT : PW_TEXT;
        tft.setTextColor(col, PW_BG);
        tft.drawString(gVendor, 120, 162);
        if (gThreat) {
            tft.setTextFont(2);
            tft.setTextSize(1);
            tft.setTextColor(PW_THREAT, PW_BG);
            tft.drawString(F("! TRACKER !"), 120, 178);
        }
    }
}

static void renderBar(int8_t rssi) {
    static const uint16_t BAR_X = 0;
    static const uint16_t BAR_Y = 190;
    static const uint16_t BAR_W = 240;
    static const uint16_t BAR_H = 30;

    RssiZone  zone     = classifyRssi(rssi);
    uint16_t  barWidth = rssiToBarWidth(rssi, BAR_W);
    uint16_t  barColor = zoneColor(zone);

    // Background track
    tft.fillRect(BAR_X, BAR_Y, BAR_W, BAR_H, PW_DIM);
    // Active bar
    if (barWidth > 0) {
        tft.fillRect(BAR_X, BAR_Y, barWidth, BAR_H, barColor);
    }
    // Border
    tft.drawRect(BAR_X, BAR_Y, BAR_W, BAR_H, PW_TEXT);

    // Zone label below bar
    tft.fillRect(0, 228, 240, 20, PW_BG);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(barColor, PW_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(zoneLabel(zone), 120, 234);
}

static void renderButtonHints() {
    tft.fillRect(0, 302, 240, 18, PW_BG);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(PW_DIM, PW_BG);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(F("UP/DOWN:mode"), 4, 307);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(F("SEL:exit"), 236, 307);
}

static void renderFull() {
    int8_t    rssi  = gRssi;
    RssiZone  zone  = classifyRssi(rssi);
    uint8_t   zoneU = static_cast<uint8_t>(zone);

    renderRssi(rssi);
    renderBar(rssi);

    if (rssi != lastRenderedRssi || zoneU != lastZone) {
        renderVendor();
        lastRenderedRssi = rssi;
        lastZone         = zoneU;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Button handling — non-blocking edge detection via PCF8574
// ─────────────────────────────────────────────────────────────────────────────
static bool prevUp     = false;
static bool prevDown   = false;
static bool prevSelect = false;

// Returns true when SELECT is released after being pressed (debounced).
static bool handleButtons(bool& exitRequested) {
    bool curUp     = !pcf.digitalRead(BTN_UP_CH);
    bool curDown   = !pcf.digitalRead(BTN_DOWN_CH);
    bool curSelect = !pcf.digitalRead(BTN_SELECT_CH);

    // Rising edge on SELECT → exit
    if (curSelect && !prevSelect) {
        exitRequested = true;
    }

    // Rising edge on UP or DOWN → mode toggle
    if ((curUp && !prevUp) || (curDown && !prevDown)) {
        scanMode = (scanMode == MODE_SUBGHZ) ? MODE_2G4 : MODE_SUBGHZ;

        // Switch SPI bus ownership between CC1101 and NRF24.
        if (scanMode == MODE_SUBGHZ) {
            proximityRadio.stopListening();
            cleanupNRF24();
            initCC1101();
        } else {
            cleanupSubGHz();
            initNRF24();
        }

        // Reset smoothing state and clear vendor label.
        gRssiSmoothed = -100;
        gVendor[0]    = '\0';
        gThreat       = false;
        lastZone      = 0xFF;   // force full redraw

        renderHeader();
    }

    prevUp     = curUp;
    prevDown   = curDown;
    prevSelect = curSelect;
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public namespace implementation
// ─────────────────────────────────────────────────────────────────────────────
namespace ProximityWand {

void setup() {
    // Release whichever radio the previous module left active.
    cleanupNRF24();
    cleanupSubGHz();
    DEASSERT_SPI_BUS();

    // Reset smoothing and scan state.
    gRssi          = -100;
    gRssiSmoothed  = -100;
    gVendor[0]     = '\0';
    gThreat        = false;
    nrfHits        = 0;
    nrfChannel     = 0;
    scanMode       = MODE_SUBGHZ;

    lastRenderedRssi = -127;  // force first full redraw
    lastZone         = 0xFF;
    lastRadioMs      = millis();
    lastRenderMs     = millis();
    prevUp = prevDown = prevSelect = false;

    // Start in Sub-GHz mode.
    initCC1101();

    // Full screen clear and static elements.
    tft.fillScreen(PW_BG);
    renderHeader();
    renderButtonHints();
}

void loop() {
    bool exitRequested = false;
    handleButtons(exitRequested);

    if (exitRequested) {
        teardown();
        return;
    }

    unsigned long now = millis();

    // ── Radio poll task ───────────────────────────────────────────────────────
    if (now - lastRadioMs >= PROX_RADIO_MS) {
        lastRadioMs = now;
        if (scanMode == MODE_SUBGHZ) {
            pollCC1101();
        } else {
            pollNRF24();
        }
    }

    // ── TFT render task (lower priority — runs less often) ────────────────────
    if (now - lastRenderMs >= PROX_RENDER_MS) {
        lastRenderMs = now;
        renderFull();
    }
}

void teardown() {
    Serial.println(F("[ProxWand] teardown — releasing SPI bus"));

    if (scanMode == MODE_2G4) {
        proximityRadio.stopListening();
        cleanupNRF24();
    } else {
        cleanupSubGHz();
    }

    DEASSERT_SPI_BUS();
    tft.fillScreen(BG_BASE);
}

} // namespace ProximityWand
