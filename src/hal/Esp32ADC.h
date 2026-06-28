// src/hal/Esp32ADC.h
#pragma once
#include <Arduino.h>
#include "hal/IADC.h"

class Esp32ADC : public IADC {
public:
    uint32_t readMillivolts(uint8_t pin) override {
        return analogReadMilliVolts(pin);
    }
    void setAttenuation(uint8_t pin, uint8_t att) override {
        analogSetPinAttenuation(pin, (adc_attenuation_t)att);
    }
};
