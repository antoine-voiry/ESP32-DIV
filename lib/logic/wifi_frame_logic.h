#pragma once
#include <stdint.h>

bool    isDeauthFrame(uint8_t frameType);
bool    isDisassocFrame(uint8_t frameType);
uint8_t wrapChannel(uint8_t ch, uint8_t minCh, uint8_t maxCh);
