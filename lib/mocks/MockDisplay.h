// lib/mocks/MockDisplay.h
#pragma once
#include "hal/IDisplay.h"

struct MockDisplay : IDisplay {
    void    fillScreen(uint16_t) override {}
    void    fillRect(int16_t, int16_t, int16_t, int16_t, uint16_t) override {}
    void    drawRect(int16_t, int16_t, int16_t, int16_t, uint16_t) override {}
    void    drawRoundRect(int16_t, int16_t, int16_t, int16_t, int16_t, uint16_t) override {}
    void    fillRoundRect(int16_t, int16_t, int16_t, int16_t, int16_t, uint16_t) override {}
    void    drawLine(int16_t, int16_t, int16_t, int16_t, uint16_t) override {}
    void    drawFastVLine(int16_t, int16_t, int16_t, uint16_t) override {}
    void    drawBitmap(int16_t, int16_t, const uint8_t*, int16_t, int16_t, uint16_t) override {}
    void    pushImage(int16_t, int16_t, int16_t, int16_t, const uint16_t*) override {}
    void    setCursor(int16_t, int16_t) override {}
    void    setTextColor(uint16_t) override {}
    void    setTextColor(uint16_t, uint16_t) override {}
    void    setTextSize(uint8_t) override {}
    void    setTextFont(uint8_t) override {}
    void    print(const char*) override {}
    void    println(const char*) override {}
    void    printf(const char*, ...) override {}
    void    setTextDatum(uint8_t) override {}
    void    drawString(const char*, int16_t, int16_t, uint8_t) override {}
    int16_t width() override  { return 240; }
    int16_t height() override { return 320; }
};
