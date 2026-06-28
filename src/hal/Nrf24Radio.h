// src/hal/Nrf24Radio.h
#pragma once
#include <RF24.h>
#include "hal/INrfRadio.h"

class Nrf24Radio : public INrfRadio {
    RF24& _r;
public:
    explicit Nrf24Radio(RF24& radio) : _r(radio) {}
    bool begin() override                                          { return _r.begin(); }
    bool write(const void* buf, uint8_t len) override              { return _r.write(buf, len); }
    bool available() override                                      { return _r.available(); }
    void read(void* buf, uint8_t len) override                     { _r.read(buf, len); }
    void openWritingPipe(const uint8_t* addr) override             { _r.openWritingPipe(addr); }
    void openReadingPipe(uint8_t num, const uint8_t* addr) override { _r.openReadingPipe(num, addr); }
    void startListening() override                                 { _r.startListening(); }
    void stopListening() override                                  { _r.stopListening(); }
};
