// src/hal/TftDisplay.cpp
#include "TftDisplay.h"
#include <cstdarg>
#include <cstdio>

void TftDisplay::printf(const char* fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    _tft.print(buf);
}
