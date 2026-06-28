// lib/logic/battery_logic.h
#pragma once
#include <stdint.h>

float computeVoltage(uint32_t avgMv);
int   computeBatteryPercent(float voltage);
int   computeWifiStrength(int rssi);
