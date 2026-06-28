#pragma once
#include <stdint.h>

struct INrfRadio {
    virtual bool begin() = 0;
    virtual bool write(const void* buf, uint8_t len) = 0;
    virtual bool available() = 0;
    virtual void read(void* buf, uint8_t len) = 0;
    virtual void openWritingPipe(const uint8_t* addr) = 0;
    virtual void openReadingPipe(uint8_t num, const uint8_t* addr) = 0;
    virtual void startListening() = 0;
    virtual void stopListening() = 0;
    virtual ~INrfRadio() = default;
};
