// lib/mocks/MockSubGhzRadio.h
#pragma once
#include "hal/ISubGhzRadio.h"

struct MockSubGhzRadio : ISubGhzRadio {
    void    init() override                           {}
    void    setFrequency(float) override              {}
    bool    sendData(uint8_t*, uint8_t) override      { return true; }
    uint8_t receiveData(uint8_t*) override            { return 0; }
    bool    checkReceiveFlag() override               { return false; }
};
