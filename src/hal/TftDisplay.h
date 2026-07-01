// src/hal/TftDisplay.h
#pragma once
#include <TFT_eSPI.h>
#include "hal/IDisplay.h"

class TftDisplay : public IDisplay {
    TFT_eSPI& _tft;
public:
    explicit TftDisplay(TFT_eSPI& tft) : _tft(tft) {}
    void    fillScreen(uint16_t c) override                                              { _tft.fillScreen(c); }
    void    fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) override  { _tft.fillRect(x,y,w,h,c); }
    void    drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) override  { _tft.drawRect(x,y,w,h,c); }
    void    drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c) override { _tft.drawRoundRect(x,y,w,h,r,c); }
    void    fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c) override { _tft.fillRoundRect(x,y,w,h,r,c); }
    void    drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c) override { _tft.drawLine(x0,y0,x1,y1,c); }
    void    drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t c) override         { _tft.drawFastVLine(x,y,h,c); }
    void    drawBitmap(int16_t x, int16_t y, const uint8_t* bm, int16_t w, int16_t h, uint16_t c) override { _tft.drawBitmap(x,y,bm,w,h,c); }
    void    pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) override { _tft.pushImage(x, y, w, h, data); }
    void    setCursor(int16_t x, int16_t y) override                                    { _tft.setCursor(x,y); }
    void    setTextColor(uint16_t c) override                                            { _tft.setTextColor(c); }
    void    setTextColor(uint16_t fg, uint16_t bg) override                             { _tft.setTextColor(fg,bg); }
    void    setTextSize(uint8_t s) override                                              { _tft.setTextSize(s); }
    void    setTextFont(uint8_t f) override                                              { _tft.setTextFont(f); }
    void    print(const char* s) override                                                { _tft.print(s); }
    void    println(const char* s) override                                              { _tft.println(s); }
    void    printf(const char* fmt, ...) override;
    void    setTextDatum(uint8_t d) override                                             { _tft.setTextDatum(d); }
    void    drawString(const char* s, int16_t x, int16_t y, uint8_t f) override        { _tft.drawString(s,x,y,f); }
    int16_t width() override                                                             { return _tft.width(); }
    int16_t height() override                                                            { return _tft.height(); }
};
