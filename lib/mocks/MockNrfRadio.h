// lib/mocks/MockNrfRadio.h
#pragma once
#include "hal/INrfRadio.h"

struct MockNrfRadio : INrfRadio {
    bool begin() override                                     { return true; }
    bool write(const void*, uint8_t) override                 { return true; }
    bool available() override                                 { return false; }
    void read(void*, uint8_t) override                        {}
    void openWritingPipe(const uint8_t*) override             {}
    void openReadingPipe(uint8_t, const uint8_t*) override    {}
    void startListening() override                            {}
    void stopListening() override                             {}
};
