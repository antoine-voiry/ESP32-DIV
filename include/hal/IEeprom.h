#pragma once
#include <stdint.h>

struct IEeprom {
    virtual uint8_t read(int addr) = 0;
    virtual void    write(int addr, uint8_t val) = 0;
    virtual bool    commit() = 0;
    virtual ~IEeprom() = default;
};
