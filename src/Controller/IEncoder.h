#pragma once

#include <Arduino.h>
#include <ESP32Encoder.h>
#include <cstdint>
#include "MCP.h"

class IEncoder {
private:
    static constexpr int kDefaultPcntUnit = 6;

    int _pinA;
    int _pinB;
    int _threshold;
    MCP* _mcp;
    bool _enabled;
    int _pcntUnit;
    ESP32Encoder* _encoder;
    int64_t _lastPosition;
    volatile int64_t _capturedCount;

    bool ready() const;

public:
    IEncoder(int pinA, int pinB, int threshold, MCP* mcp = nullptr, bool enabled = true, int pcntUnit = kDefaultPcntUnit);
    ~IEncoder();

    // Обертки над публичными методами ESP32Encoder.
    void attachHalfQuad(int pcnt_unit = kDefaultPcntUnit);
    void attachFullQuad(int pcnt_unit = kDefaultPcntUnit);
    void attachSingleEdge(int pcnt_unit = kDefaultPcntUnit);
    int64_t getCount();
    int64_t clearCount();
    int64_t pauseCount();
    int64_t resumeCount();
    void IRAM_ATTR setCapture(int64_t value);
    int64_t getCapture() const;
    void resetCapture();
    void detach();
    bool isAttached() const;
    void setCount(int64_t value);
    void setFilter(uint16_t value);

    // Обертки над статическими настройками библиотеки.
    static void setUseInternalWeakPullResistors(puType type);
    static puType getUseInternalWeakPullResistors();
    static void setIsrServiceCpuCore(uint32_t core);
    static uint32_t getIsrServiceCpuCore();

    bool isEnabled() const { return _enabled; }
    int getPcntUnit() const { return _pcntUnit; }
    int getThreshold() const { return _threshold; }
};
