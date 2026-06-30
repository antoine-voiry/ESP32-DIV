// lib/logic/battery_logic.cpp
#include "battery_logic.h"
#include <math.h>

float computeVoltage(uint32_t avgMv) {
    return (avgMv / 1000.0f) * 2.0f;
}

int computeBatteryPercent(float voltage) {
    long v = (long)lroundf(voltage * 100.0f);
    long mapped = (v - 300L) * 100L / (420L - 300L);
    if (mapped < 0)   return 0;
    if (mapped > 100) return 100;
    return (int)mapped;
}

int computeWifiStrength(int rssi) {
    long mapped = (long)(rssi - (-100)) * 100L / (-30 - (-100));
    if (mapped < 0)   return 0;
    if (mapped > 100) return 100;
    return (int)mapped;
}

int computeTempStatus(float tempC) {
    if (tempC >= 80.0f) return 2;
    if (tempC >= 65.0f) return 1;
    return 0;
}
