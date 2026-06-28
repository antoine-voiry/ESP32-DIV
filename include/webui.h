#pragma once
#include <Arduino.h>

namespace WebUIService {
    void setup();
    void loop();
    void teardown();
    bool isActive();
    void broadcastEvent(const String& json);
}
