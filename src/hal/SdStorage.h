// src/hal/SdStorage.h
#pragma once
#include <SD.h>
#include "hal/IStorage.h"

class SdStorage : public IStorage {
public:
    bool begin() override                  { return SD.begin(); }
    bool exists(const char* path) override { return SD.exists(path); }
};
