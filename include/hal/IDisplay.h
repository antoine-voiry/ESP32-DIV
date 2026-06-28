#pragma once
#include <stdint.h>

struct IDisplay {
    virtual void    fillScreen(uint16_t color) = 0;
    virtual void    fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void    drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void    drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) = 0;
    virtual void    fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) = 0;
    virtual void    drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) = 0;
    virtual void    drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) = 0;
    virtual void    drawBitmap(int16_t x, int16_t y, const uint8_t* bm,
                               int16_t w, int16_t h, uint16_t color) = 0;
    virtual void    setCursor(int16_t x, int16_t y) = 0;
    virtual void    setTextColor(uint16_t color) = 0;
    virtual void    setTextColor(uint16_t fgcolor, uint16_t bgcolor) = 0;
    virtual void    setTextSize(uint8_t size) = 0;
    virtual void    setTextFont(uint8_t font) = 0;
    virtual void    print(const char* s) = 0;
    virtual void    println(const char* s) = 0;
    virtual void    printf(const char* fmt, ...) = 0;
    virtual void    setTextDatum(uint8_t datum) = 0;
    virtual void    drawString(const char* s, int16_t x, int16_t y, uint8_t font) = 0;
    virtual int16_t width() = 0;
    virtual int16_t height() = 0;
    virtual ~IDisplay() = default;
};
