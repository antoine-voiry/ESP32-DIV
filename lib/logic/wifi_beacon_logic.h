#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

uint8_t channelDown(uint8_t ch, uint8_t maxCh);
uint8_t channelUp(uint8_t ch, uint8_t maxCh);
bool    toggleAttack(bool current);
int     buildBeaconTlv(uint8_t* packet, size_t bufLen,
                       const char* ssid, uint8_t ssidLen,
                       uint8_t channel);
