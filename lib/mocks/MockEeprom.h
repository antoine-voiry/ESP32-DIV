// lib/mocks/MockEeprom.h
#pragma once
#include "hal/IEeprom.h"
#include <stdint.h>

struct MockEeprom : IEeprom {
    uint8_t mem[16] = {};
    uint8_t read(int addr) override              { return mem[addr]; }
    void    write(int addr, uint8_t val) override { mem[addr] = val; }
    bool    commit() override                    { return true; }
};
