#pragma once

struct IStorage {
    virtual bool begin() = 0;
    virtual bool exists(const char* path) = 0;
    virtual ~IStorage() = default;
};
