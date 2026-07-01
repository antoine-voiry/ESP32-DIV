// lib/logic/battery_logic.h
#pragma once
#include <stdint.h>

float computeVoltage(uint32_t avgMv);
int   computeBatteryPercent(float voltage);
int   computeWifiStrength(int rssi);

// Returns 0=OK, 1=WARN, 2=ERR based on ESP32 die temperature (°C).
// Thresholds calibrated for die temperature: idle ~50°C, heavy load ~80°C.
int   computeTempStatus(float tempC);

// Returns display WiFi strength 0–100.
// Connected → maps rssi via computeWifiStrength; AP mode → 100; neither → 0.
// isConnected takes priority over isAp.
int computeWifiDisplayStrength(bool isConnected, bool isAp, int rssi);
