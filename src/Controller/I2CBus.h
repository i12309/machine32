#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class I2CBus {
public:
    static bool init();
    static bool lock(TickType_t timeoutTicks = pdMS_TO_TICKS(5));
    static bool tryLock() { return lock(0); }
    static void unlock();

private:
    static SemaphoreHandle_t mutex;
    static bool initialized;
};

