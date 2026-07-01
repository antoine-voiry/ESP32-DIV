// src/hal/CC1101Radio.h
#pragma once
#include <ELECHOUSE_CC1101_ESP32DIV.h>
#include "hal/ISubGhzRadio.h"
#include "subghz_logic.h"

class CC1101Radio : public ISubGhzRadio {
    bool _present = false;
public:
    void init() override {
        ELECHOUSE_cc1101.Init();
        _present = ELECHOUSE_cc1101.getCC1101();
    }
    bool    isPresent() override                             { return _present; }
    void    setFrequency(float mhz) override                 { ELECHOUSE_cc1101.setMHZ(mhz); }
    bool    sendData(uint8_t* data, uint8_t len) override    { ELECHOUSE_cc1101.SendData(data, len); return true; }
    uint8_t receiveData(uint8_t* data) override              { return ELECHOUSE_cc1101.ReceiveData(data); }
    bool    checkReceiveFlag() override                      { return ELECHOUSE_cc1101.CheckReceiveFlag(); }
};
