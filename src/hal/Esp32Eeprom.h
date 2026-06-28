// src/hal/Esp32Eeprom.h
#pragma once
#include <EEPROM.h>
#include "hal/IEeprom.h"

class Esp32Eeprom : public IEeprom {
public:
    uint8_t read(int addr) override              { return EEPROM.read(addr); }
    void    write(int addr, uint8_t val) override { EEPROM.write(addr, val); }
    bool    commit() override                    { return EEPROM.commit(); }
};
