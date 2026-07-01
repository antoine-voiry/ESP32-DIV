// lib/mocks/MockSubGhzRadio.h
#pragma once
#include "hal/ISubGhzRadio.h"

struct MockSubGhzRadio : ISubGhzRadio {
    bool present = true;   // set to false in tests to simulate absent chip
    void    init() override                           {}
    bool    isPresent() override                      { return present; }
    void    setFrequency(float) override              {}
    bool    sendData(uint8_t*, uint8_t) override      { return true; }
    uint8_t receiveData(uint8_t*) override            { return 0; }
    bool    checkReceiveFlag() override               { return false; }
};
