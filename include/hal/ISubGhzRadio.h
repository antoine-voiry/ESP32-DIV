#pragma once
#include <stdint.h>

struct ISubGhzRadio {
    virtual void    init() = 0;
    virtual void    setFrequency(float mhz) = 0;
    virtual bool    sendData(uint8_t* data, uint8_t len) = 0;
    virtual uint8_t receiveData(uint8_t* data) = 0;
    virtual bool    checkReceiveFlag() = 0;
    virtual bool    isPresent() = 0;
    virtual ~ISubGhzRadio() = default;
};
