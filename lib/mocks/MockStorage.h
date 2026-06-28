// lib/mocks/MockStorage.h
#pragma once
#include "hal/IStorage.h"

struct MockStorage : IStorage {
    bool beginResult  = true;
    bool existsResult = false;
    bool begin() override               { return beginResult; }
    bool exists(const char*) override   { return existsResult; }
};
