#include "wifi_beacon_logic.h"
#include <string.h>

uint8_t channelDown(uint8_t ch, uint8_t maxCh) {
    return (ch == 1) ? maxCh : (uint8_t)(ch - 1);
}

uint8_t channelUp(uint8_t ch, uint8_t maxCh) {
    return (ch == maxCh) ? (uint8_t)1 : (uint8_t)(ch + 1);
}

bool toggleAttack(bool current) {
    return !current;
}

int buildBeaconTlv(uint8_t* packet, size_t bufLen,
                   const char* ssid, uint8_t ssidLen,
                   uint8_t channel) {
    int dsOffset = 38 + (int)ssidLen + 10;
    int totalLen = dsOffset + 3;
    if ((size_t)totalLen > bufLen) return -1;

    packet[37] = ssidLen;
    for (int i = 0; i < (int)ssidLen; i++) {
        packet[38 + i] = (uint8_t)ssid[i];
    }

    int ratesOffset = 38 + (int)ssidLen;
    packet[ratesOffset]     = 0x01;
    packet[ratesOffset + 1] = 0x08;
    packet[ratesOffset + 2] = 0x82; packet[ratesOffset + 3] = 0x84;
    packet[ratesOffset + 4] = 0x8b; packet[ratesOffset + 5] = 0x96;
    packet[ratesOffset + 6] = 0x24; packet[ratesOffset + 7] = 0x30;
    packet[ratesOffset + 8] = 0x48; packet[ratesOffset + 9] = 0x6c;

    packet[dsOffset]     = 0x03;
    packet[dsOffset + 1] = 0x01;
    packet[dsOffset + 2] = channel;

    return totalLen;
}
