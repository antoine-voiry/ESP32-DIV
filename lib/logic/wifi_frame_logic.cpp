#include "wifi_frame_logic.h"

bool isDeauthFrame(uint8_t frameType)   { return frameType == 0xC0; }
bool isDisassocFrame(uint8_t frameType) { return frameType == 0xA0; }

uint8_t wrapChannel(uint8_t ch, uint8_t minCh, uint8_t maxCh) {
    if (ch < minCh || ch > maxCh) return minCh;
    return ch;
}
