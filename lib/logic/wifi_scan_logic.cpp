#include "wifi_scan_logic.h"
#include <string.h>

NetworkAnomaly analyzeAp(const WifiApInfo* ap,
                         const WifiApInfo* allAps, int apCount,
                         int deauthCount) {
    NetworkAnomaly result = {false, false, false, false};
    result.isHidden      = (ap->ssid[0] == '\0');
    result.isWeirdChannel = (ap->channel > 13);
    result.isUnderAttack  = (deauthCount > 5);

    for (int j = 0; j < apCount; j++) {
        if (memcmp(ap->bssid, allAps[j].bssid, 6) == 0) continue;
        if (strcmp(ap->ssid, allAps[j].ssid) == 0) {
            result.isEvilTwin = true;
            break;
        }
    }
    return result;
}

int compareApByRssi(const void* a, const void* b) {
    const WifiApInfo* ap1 = (const WifiApInfo*)a;
    const WifiApInfo* ap2 = (const WifiApInfo*)b;
    return (int)ap2->rssi - (int)ap1->rssi;
}
