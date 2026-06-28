#pragma once
#include <stdint.h>

struct IADC {
    virtual uint32_t readMillivolts(uint8_t pin) = 0;
    virtual void     setAttenuation(uint8_t pin, uint8_t attenuation) = 0;
    virtual ~IADC() = default;
};
