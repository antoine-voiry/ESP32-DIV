/*
ELECHOUSE_CC1101_ESP32DIV.h
==============================================================
ESP32-DIV COMPATIBILITY WRAPPER

This header provides compatibility for ESP32-DIV legacy code
that uses #include <ELECHOUSE_CC1101_ESP32DIV.h>

It redirects to the SmartRC-CC1101-Driver-Lib which provides
the ELECHOUSE_CC1101_SRC_DRV interface.
==============================================================
*/

#ifndef ELECHOUSE_CC1101_ESP32DIV_h
#define ELECHOUSE_CC1101_ESP32DIV_h

// Redirect to SmartRC CC1101 driver (provides ELECHOUSE_CC1101_SRC_DRV.h)
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#endif
