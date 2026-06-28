// lib/mocks/MockADC.h
#pragma once
#include "hal/IADC.h"

struct MockADC : IADC {
    uint32_t nextReading = 0;
    uint32_t readMillivolts(uint8_t) override      { return nextReading; }
    void     setAttenuation(uint8_t, uint8_t) override {}
};
