#include "utils.h"
#include "shared.h"
#include "icon.h"
#include "Touchscreen.h"
#include <WiFi.h>  // For WiFi.RSSI() and WiFi.status()
#include "hal/hal_globals.h"
#include "hal/IADC.h"
#include "battery_logic.h"


/*
 * 
 * Notification
 * 
 */


/*
    showNotification("New Message!", "Task Failed Successfully.");
    
    if (notificationVisible && ts.touched()) {
      int x, y, z;
        TS_Point p = ts.getPoint();
        x = ::map(p.x, TS_MINX, TS_MAXX, 0, DISPLAY_WIDTH - 1);
        y = ::map(p.y, TS_MAXY, TS_MINY, 0, DISPLAY_HEIGHT - 1);
        
    if (x >= closeButtonX && x <= (closeButtonX + closeButtonSize) &&
        y >= closeButtonY && y <= (closeButtonY + closeButtonSize)) {
        hideNotification();
    }
    
    if (x >= okButtonX && x <= (okButtonX + okButtonWidth) &&
        y >= okButtonY && y <= (okButtonY + okButtonHeight)) {
        hideNotification();
    }
     delay(100);
  }
  
*/

bool notificationVisible = true;
int notifX, notifY, notifWidth, notifHeight;
int closeButtonX, closeButtonY, closeButtonSize = 15;
int okButtonX, okButtonY, okButtonWidth = 60, okButtonHeight = 20;


void showNotification(const char* title, const char* message) {
    notifWidth = 200;
    notifHeight = 80;
    notifX = (240 - notifWidth) / 2;
    notifY = (320 - notifHeight) / 2;

    tft.fillRect(notifX, notifY, notifWidth, notifHeight, LIGHT_GRAY);
    tft.fillRect(notifX, notifY, notifWidth, 20, BLUE);
    
    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    tft.setCursor(notifX + 5, notifY + 5);
    tft.print(title);

    closeButtonX = notifX + notifWidth - closeButtonSize - 5;
    closeButtonY = notifY + 2;
    tft.fillRect(closeButtonX, closeButtonY, closeButtonSize, closeButtonSize, RED);
    tft.setTextColor(WHITE);
    tft.setCursor(closeButtonX + 5, closeButtonY + 4);
    tft.print("X");

    int messageBoxX = notifX + 5;
    int messageBoxY = notifY + 25;
    int messageBoxWidth = notifWidth - 10;
    int messageBoxHeight = notifHeight - 45;

    tft.fillRect(messageBoxX, messageBoxY, messageBoxWidth, messageBoxHeight, WHITE);
    tft.setTextColor(BLACK);
    printWrappedText(messageBoxX + 2, messageBoxY + 5, messageBoxWidth + 2, message);

    okButtonX = notifX + (notifWidth - okButtonWidth) / 2;
    okButtonY = notifY + notifHeight - 25;

    tft.fillRect(okButtonX, okButtonY, okButtonWidth, okButtonHeight, GRAY);
    tft.drawRect(okButtonX, okButtonY, okButtonWidth, okButtonHeight, DARK_GRAY);
    tft.drawLine(okButtonX, okButtonY, okButtonX + okButtonWidth, okButtonY, WHITE);
    tft.drawLine(okButtonX, okButtonY, okButtonX, okButtonY + okButtonHeight, WHITE);

    tft.setTextColor(BLACK);
    tft.setCursor(okButtonX + 20, okButtonY + 5);
    tft.print("OK");

    notificationVisible = true;
}

void hideNotification() {
    tft.fillRect(notifX, notifY, notifWidth, notifHeight, BLACK);
    notificationVisible = false;
}

void printWrappedText(int x, int y, int maxWidth, const char* text) {
    String message = text;  
    int cursorX = x, cursorY = y;
    
    while (message.length() > 0) {
        int lineEnd = message.length();
        
        while (tft.textWidth(message.substring(0, lineEnd)) > maxWidth) {
            lineEnd--;
        }

        if (lineEnd < message.length()) {
            int lastSpace = message.substring(0, lineEnd).lastIndexOf(' ');
            if (lastSpace > 0) lineEnd = lastSpace;
        }

        tft.setCursor(cursorX, cursorY);
        tft.print(message.substring(0, lineEnd));

        message = message.substring(lineEnd);
        message.trim();

        cursorY += 15;
    }
}


/*
 * 
 * StatusBar
 * 
 */

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif

unsigned long lastStatusBarUpdate = 0;
const int STATUS_BAR_UPDATE_INTERVAL = 1000; 
float lastBatteryVoltage = 0.0;

float readBatteryVoltage() {
    static bool adcInitialized = false;
    if (!adcInitialized) {
        gADC->setAttenuation(36, 3);  // 3 == ADC_11db
        adcInitialized = true;
        Serial.println("[BATTERY] ADC initialized on GPIO36 with 11dB attenuation");
    }

    const int sampleCount = 16;
    uint32_t sum = 0;
    for (int i = 0; i < sampleCount; i++) {
        sum += gADC->readMillivolts(36);
        delayMicroseconds(500);
    }

    float avgMv = (float)sum / sampleCount;
    float voltage = computeVoltage((uint32_t)avgMv);

    static unsigned long lastDebugPrint = 0;
    if (millis() - lastDebugPrint > 5000) {
        Serial.printf("[BATTERY] Raw mV: %.1f, Voltage: %.2fV\n", avgMv, voltage);
        if (avgMv < 100) {
            Serial.println("[BATTERY] WARNING: check GPIO36 wiring!");
        }
        lastDebugPrint = millis();
    }
    return voltage;
}

float readInternalTemperature() {
  uint8_t raw = temprature_sens_read();
  return ((raw - 32) / 1.8);
}

// Check if SD card is available
bool isSDCardAvailable() {
  return SD.begin();
}

void drawStatusBar(float batteryVoltage, bool forceUpdate) {
  static int lastBatteryPercentage = -1;
  static int lastWiFiStrength = -1;

    int batteryPercentage = computeBatteryPercent(batteryVoltage);

    int wifiStrength = 0;
    wifi_mode_t wifiMode = WiFi.getMode();
    if (WiFi.status() == WL_CONNECTED) {
        wifiStrength = computeWifiStrength(WiFi.RSSI());
    } else if (wifiMode == WIFI_AP || wifiMode == WIFI_AP_STA) {
        wifiStrength = 100;
    }

  float internalTemp = readInternalTemperature();
  bool sdAvailable = false;

  if (batteryPercentage != lastBatteryPercentage || wifiStrength != lastWiFiStrength || forceUpdate) {
    int barHeight = 24;
    int x = 7;
    int y = 4;

    tft.fillRect(0, 0, tft.width(), barHeight, BG_SURFACE);

    // Battery outline + terminal
    tft.drawRoundRect(x, y, 22, 10, 2, ACCENT_BORDER);
    tft.fillRect(x + 22, y + 3, 2, 4, ACCENT_BORDER);

    // Battery fill: 3-tier color
    int batteryLevelWidth = map(batteryPercentage, 0, 100, 0, 20);
    uint16_t batteryColor = (batteryPercentage > 50) ? STATUS_OK
                          : (batteryPercentage > 20) ? STATUS_WARN : STATUS_ERR;
    tft.fillRoundRect(x + 2, y + 2, batteryLevelWidth, 6, 1, batteryColor);

    // Battery percentage text
    tft.setCursor(x + 30, y + 2);
    tft.setTextColor(TEXT_PRIMARY, BG_SURFACE);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.print(String(batteryPercentage) + "%");

    // Wi-Fi signal bars
    int wifiX = 180;
    int wifiY = y + 11;
    for (int i = 0; i < 4; i++) {
      int barH = (i + 1) * 3;
      int barWidth = 4;
      int barX = wifiX + i * 6;
      if (wifiStrength > i * 25) {
        tft.fillRoundRect(barX, wifiY - barH, barWidth, barH, 1, STATUS_OK);
      } else {
        tft.drawRoundRect(barX, wifiY - barH, barWidth, barH, 1, ACCENT_BORDER);
      }
    }

    // Temperature icon — thresholds via computeTempStatus (die temperature)
    {
      int ts = computeTempStatus(internalTemp);
      uint16_t tc = (ts == 2) ? STATUS_ERR : (ts == 1) ? STATUS_WARN : STATUS_OK;
      tft.drawBitmap(203, y - 3, bitmap_icon_temp, 16, 16, tc);
    }

    // SD card icon
    if (sdAvailable) {
      tft.drawBitmap(220, y - 3, bitmap_icon_sdcard, 16, 16, STATUS_OK);
    } else {
      tft.drawBitmap(220, y - 3, bitmap_icon_nullsdcard, 16, 16, STATUS_ERR);
    }

    lastBatteryPercentage = batteryPercentage;
    lastWiFiStrength = wifiStrength;
  }
}

void updateStatusBar() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastStatusBarUpdate > STATUS_BAR_UPDATE_INTERVAL) {
    float batteryVoltage = readBatteryVoltage();

    if (abs(batteryVoltage - lastBatteryVoltage) > 0.05 || lastBatteryVoltage == 0) {
      drawStatusBar(batteryVoltage);
      lastBatteryVoltage = batteryVoltage;
    }

    lastStatusBarUpdate = currentMillis;
  }
}


/*
 * 
 * Loading
 * 
 */

void loading(int frameDelay, uint16_t color, int16_t x, int16_t y, int repeats, bool center) {
  int16_t bitmapWidth = 100;
  int16_t bitmapHeight = 120;
  int16_t logoX = x;
  int16_t logoY = y;
  int16_t screenWidth = tft.width();
  int16_t screenHeight = tft.height();

  if (center) {
    logoX = (screenWidth - bitmapWidth) / 2;
    logoY = (screenHeight - bitmapHeight) / 2 - 25;  // Move up for text
  }

  // Array of bitmaps
  const unsigned char* bitmaps[] = {
    bitmap_icon_hal9000_loading_1,
    bitmap_icon_hal9000_loading_2,
    bitmap_icon_hal9000_loading_3,
    bitmap_icon_hal9000_loading_4,
    bitmap_icon_hal9000_loading_5,
    bitmap_icon_hal9000_loading_6,
    bitmap_icon_hal9000_loading_7,
    bitmap_icon_hal9000_loading_8,
    bitmap_icon_hal9000_loading_9,
    bitmap_icon_hal9000_loading_10
  };
  const int numFrames = 10;

  for (int r = 0; r < repeats; r++) {
    for (int i = 0; i < numFrames; i++) {
      uint16_t frameColor = STATUS_ERR;  // HAL red

      // Clear skull area
      tft.fillRect(logoX, logoY, bitmapWidth, bitmapHeight + 40, TFT_BLACK);

      // Draw skull frame
      tft.drawBitmap(logoX, logoY, bitmaps[i], bitmapWidth, bitmapHeight, frameColor);

      // Draw title text below loading animation
      if (center) {
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(frameColor, TFT_BLACK);

        const char* text = "ESP32-DIV";
        int16_t textW = tft.textWidth(text);
        int16_t textX = (screenWidth - textW) / 2;
        int16_t textY = logoY + bitmapHeight + 10;

        tft.setCursor(textX, textY);
        tft.print(text);
      }

      delay(frameDelay);
    }
  }
}


/*
 * 
 * Display Logo
 * 
 */

void displayLogo(uint16_t color, int displayTime) {
  int16_t screenWidth = tft.width();
  int16_t screenHeight = tft.height();

  // Clear screen with black
  tft.fillScreen(TFT_BLACK);

  // Draw HAL9000 splash panel in red
  tft.drawBitmap(0, 0, bitmap_hal9000_splash, HAL9000_SPLASH_WIDTH, HAL9000_SPLASH_HEIGHT, STATUS_ERR);

  // Draw branding text in HAL red
  tft.setTextColor(STATUS_ERR);

  // ESP32-DIV title - larger
  tft.setTextFont(4);  // Font 4 = 26px, clean and sharp
  tft.setTextSize(1);
  String title = "ESP32-DIV";
  int16_t titleWidth = tft.textWidth(title);
  tft.setCursor((screenWidth - titleWidth) / 2, 255);
  tft.print(title);

  // Version line
  tft.setTextFont(2);  // Font 2 = 16px
  String version = "v0.1.0 - HAL9000 Edition";
  int16_t versionWidth = tft.textWidth(version);
  tft.setCursor((screenWidth - versionWidth) / 2, 285);
  tft.print(version);

  // Credit line
  tft.setTextFont(2);
  String credit = "By: AVo";
  int16_t creditWidth = tft.textWidth(credit);
  tft.setCursor((screenWidth - creditWidth) / 2, 303);
  tft.print(credit);

  Serial.println("==========================================");
  Serial.println("ESP32-DIV v0.1.0 - HAL9000 Edition        ");
  Serial.println("Developed by: AVo                         ");
  Serial.println("Inspired by:  HaleHound (JMFH)            ");
  Serial.println("Original by:  CiferTech                   ");
  Serial.println("==========================================");

  delay(displayTime);
}


/*
 * 
 * Terminal
 * 
 */

namespace Terminal {

#define TEXT_HEIGHT 16
#define BOT_FIXED_AREA 0
#define TOP_FIXED_AREA 86
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 320
#define SCREEN_WIDTH 240
#define SCREENHEIGHT 320

static bool uiDrawn = false;

uint16_t yStart = TOP_FIXED_AREA;
uint16_t yArea = DISPLAY_HEIGHT - TOP_FIXED_AREA - BOT_FIXED_AREA;
uint16_t yDraw = DISPLAY_HEIGHT - BOT_FIXED_AREA - TEXT_HEIGHT;

uint16_t xPos = 0;

byte data = 0;

boolean change_colour = 1;
boolean selected = 1;
boolean terminalActive = true;

int blank[19];

long baudRates[] = {9600, 19200, 38400, 57600, 115200};
byte baudIndex = 0;

void runUI() {

    #define STATUS_BAR_Y_OFFSET 20
    #define STATUS_BAR_HEIGHT 16
    #define ICON_SIZE 16
    #define ICON_NUM 3 
    
    static int iconX[ICON_NUM] = {210, 170, 10}; 
    static int iconY = STATUS_BAR_Y_OFFSET;
    
    static const unsigned char* icons[ICON_NUM] = {
        bitmap_icon_sort_up_plus,    
        bitmap_icon_power,      
        bitmap_icon_go_back 
    };

    if (!uiDrawn) {
        tft.drawLine(0, 19, 240, 19, SHREDDY_TEAL);
        tft.fillRect(0, STATUS_BAR_Y_OFFSET, SCREEN_WIDTH, STATUS_BAR_HEIGHT, DARK_GRAY);
        
        for (int i = 0; i < ICON_NUM; i++) {
            if (icons[i] != NULL) {  
                tft.drawBitmap(iconX[i], iconY, icons[i], ICON_SIZE, ICON_SIZE, SHREDDY_TEAL);
            } 
        }
        tft.drawLine(0, STATUS_BAR_Y_OFFSET + STATUS_BAR_HEIGHT, SCREEN_WIDTH, STATUS_BAR_Y_OFFSET + STATUS_BAR_HEIGHT, ORANGE);
        uiDrawn = true;               
    }

    static unsigned long lastAnimationTime = 0;
    static int animationState = 0;  
    static int activeIcon = -1;

    if (animationState > 0 && millis() - lastAnimationTime >= 150) {
        if (animationState == 1) {
            tft.drawBitmap(iconX[activeIcon], iconY, icons[activeIcon], ICON_SIZE, ICON_SIZE, SHREDDY_TEAL);
            animationState = 2;

            switch (activeIcon) {
                case 0:
                  if (terminalActive) {
                    terminalActive = false;
                  } else if (!terminalActive) {
                    baudIndex = (baudIndex + 1) % 5;
                    Serial.end();
                    delay(100);
                    Serial.begin(baudRates[baudIndex]);
                    tft.fillRect(0, 37, DISPLAY_WIDTH, 16, ORANGE);
                    tft.setTextColor(SHREDDY_TEAL, SHREDDY_TEAL);
                    String baudMsg = " Serial Terminal - " + String(baudRates[baudIndex]) + " baud ";
                    tft.drawCentreString(baudMsg, DISPLAY_WIDTH / 2, 37, 2);
                    delay(10);
                  }
                    break;
                case 1: 
                    delay(10);
                    tft.fillRect(0, 37, DISPLAY_WIDTH, 16, ORANGE);
                    tft.setTextColor(SHREDDY_TEAL, SHREDDY_TEAL);
                    tft.drawCentreString(" Serial Terminal Active ", DISPLAY_WIDTH / 2, 37, 2);
                    terminalActive = true;
                    break;
  
                case 2: 
                    feature_exit_requested = true;
                    break;
            }
        } else if (animationState == 2) {
            animationState = 0;
            activeIcon = -1;
        }
        lastAnimationTime = millis();  
    }

    static unsigned long lastTouchCheck = 0;
    const unsigned long touchCheckInterval = 50; 

    if (millis() - lastTouchCheck >= touchCheckInterval) {
        if (ts.touched() && feature_active) { 
            TS_Point p = ts.getPoint();
            int x = ::map(p.x, TS_MINX, TS_MAXX, 0, SCREEN_WIDTH - 1);
            int y = ::map(p.y, TS_MAXY, TS_MINY, 0, SCREENHEIGHT - 1);

            if (y > STATUS_BAR_Y_OFFSET && y < STATUS_BAR_Y_OFFSET + STATUS_BAR_HEIGHT) {
                for (int i = 0; i < ICON_NUM; i++) {
                    if (x > iconX[i] && x < iconX[i] + ICON_SIZE) {
                        if (icons[i] != NULL && animationState == 0) {
                            tft.drawBitmap(iconX[i], iconY, icons[i], ICON_SIZE, ICON_SIZE, TFT_BLACK);
                            animationState = 1;
                            activeIcon = i;
                            lastAnimationTime = millis();
                        }
                        break;
                    }
                }
            }
        }
        lastTouchCheck = millis();
    }
}

void scrollAddress(uint16_t vsp) {
  tft.writecommand(ILI9341_VSCRSADD);
  tft.writedata(vsp >> 8);
  tft.writedata(vsp);
}

int scroll_line() {
  int yTemp = yStart;
  tft.fillRect(0, yStart, blank[(yStart - TOP_FIXED_AREA) / TEXT_HEIGHT], TEXT_HEIGHT, TFT_BLACK);

  yStart += TEXT_HEIGHT;
  if (yStart >= DISPLAY_HEIGHT - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - DISPLAY_HEIGHT + BOT_FIXED_AREA);
  scrollAddress(yStart);
  delay(1);
  return yTemp;
}

void setupScrollArea(uint16_t tfa, uint16_t bfa) {
  tft.writecommand(ILI9341_VSCRDEF);
  tft.writedata(tfa >> 8);
  tft.writedata(tfa);
  tft.writedata((DISPLAY_HEIGHT - tfa - bfa) >> 8);
  tft.writedata(DISPLAY_HEIGHT - tfa - bfa);
  tft.writedata(bfa >> 8);
  tft.writedata(bfa);
}

void terminalSetup() {

  setupTouchscreen();
  tft.fillScreen(TFT_BLACK); 

  tft.fillRect(0, 37, DISPLAY_WIDTH, 16, ORANGE);
  tft.setTextColor(SHREDDY_TEAL, SHREDDY_TEAL);
  String baudMsg = " Serial Terminal - " + String(baudRates[baudIndex]) + " baud ";
  tft.drawCentreString(baudMsg, DISPLAY_WIDTH / 2, 37, 2);
  
  float currentBatteryVoltage = readBatteryVoltage();
  drawStatusBar(currentBatteryVoltage, false);

  uiDrawn = false;

  Serial.begin(baudRates[baudIndex]);

  setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);

  for (byte i = 0; i < 19; i++) blank[i] = 0;

}

void terminalLoop() {

  runUI();

  if (terminalActive) {
    byte charCount = 0;
    while (Serial.available() && charCount < 10) {
      data = Serial.read();
      if (data == '\r' || xPos > 231) {
        xPos = 0;
        yDraw = scroll_line();
      }
      if (data > 31 && data < 128) {
        xPos += tft.drawChar(data, xPos, yDraw, 2);
        blank[(18 + (yStart - TOP_FIXED_AREA) / TEXT_HEIGHT) % 19] = xPos;
      }
      charCount++;
      }
    }
  }
}

bool isTouchLeft() {
    if (!ts.touched()) return false;
    TS_Point p = ts.getPoint();
    int x = map(p.x, TS_MINX, TS_MAXX, 0, 239);
    return x < 40;
}
