// lib/logic/wifi_packet_logic.cpp
#include "wifi_packet_logic.h"

double computePacketGraphScale(const uint32_t* counts, size_t len, double displayHeight) {
    uint32_t maxVal = 1;
    for (size_t i = 0; i < len; i++) {
        if (counts[i] > maxVal) maxVal = counts[i];
    }
    if ((double)maxVal > displayHeight) return displayHeight / (double)maxVal;
    return 1.0;
}
