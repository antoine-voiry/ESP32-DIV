// lib/mocks/Arduino.h — minimal shim for native test builds
// Satisfies PROGMEM-decorated bitmap headers without pulling in the ESP32 SDK.
#pragma once
#include <stdint.h>
#define PROGMEM
