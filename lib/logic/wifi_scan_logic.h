#pragma once
#include <stdint.h>
#include <stdbool.h>

struct WifiApInfo {
    char    ssid[33];   // null-terminated; ssid[0]=='\0' means hidden
    uint8_t bssid[6];
    int8_t  rssi;       // dBm
    uint8_t channel;
};

struct NetworkAnomaly {
    bool isHidden;       // ssid[0] == '\0'
    bool isEvilTwin;     // same SSID, different BSSID in allAps[]
    bool isWeirdChannel; // channel > 13
    bool isUnderAttack;  // deauthCount > 5
};

NetworkAnomaly analyzeAp(const WifiApInfo* ap,
                         const WifiApInfo* allAps, int apCount,
                         int deauthCount);

int compareApByRssi(const void* a, const void* b);
