#include "I2CBus.h"

#include "Service/Log.h"

SemaphoreHandle_t I2CBus::mutex = nullptr;
bool I2CBus::initialized = false;

bool I2CBus::init() {
    if (initialized) {
        return mutex != nullptr;
    }

    // Explicit I2C configuration for board wiring.
    Wire.begin(21, 22);
    Wire.setClock(100000);
    Wire.setTimeOut(50); // проверить почему так много 

    mutex = xSemaphoreCreateMutex();
    initialized = true;

    if (mutex == nullptr) {
        Log::L("[I2C] init failed: xSemaphoreCreateMutex returned nullptr");
        return false;
    }

    Log::L("[I2C] initialized SDA=21 SCL=22 clock=100000 timeout=50ms");
    return true;
}

bool I2CBus::lock(TickType_t timeoutTicks) {
    if (!initialized && !init()) return false;
    if (mutex == nullptr) return false;
    // В ISR-контексте FreeRTOS mutex брать нельзя.
    // В ISR просто пропускаем доступ к шине, без Wire-вызовов.
    if (xPortInIsrContext()) return false;
    return xSemaphoreTake(mutex, timeoutTicks) == pdTRUE;
}

void I2CBus::unlock() {
    if (xPortInIsrContext()) return;
    if (mutex != nullptr) {
        xSemaphoreGive(mutex);
    }
}

