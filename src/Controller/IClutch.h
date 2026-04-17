#pragma once

#include "Service/Log.h"
#include "MCP.h"
#include "I2CBus.h"

class IClutch {
    int _clutchPin;
    MCP* _mcp;
    bool _enabled = true;
public:
    bool power = false; // признак включения

    IClutch(int clutchPin, MCP* mcp = nullptr, bool enabled = true) : 
    _clutchPin(clutchPin),
    _mcp(mcp),
    _enabled(enabled) {
        if (!_enabled) return;
        if (mcp == nullptr) pinMode(_clutchPin, OUTPUT);
        else if (I2CBus::lock()) {
            _mcp->pinMode(_clutchPin, OUTPUT);
            I2CBus::unlock();
        }
    };

    void switch_c(){
        if (!_enabled) return;
        if (power) disengage(); else engage();
    }

    void engage(){
        if (!_enabled) return;
        if (!power) {
            if (_mcp == nullptr) digitalWrite(_clutchPin, HIGH);
            else {
                if (!I2CBus::lock(pdMS_TO_TICKS(2))) return;
                _mcp->digitalWrite(_clutchPin, LOW); //HIGH);
                I2CBus::unlock();
            }
            power = true;
            delay(10);
        }
    };

    void disengage(){
        if (!_enabled) return;
        if (power) {
            if (_mcp == nullptr) digitalWrite(_clutchPin, LOW);
            else {
                if (!I2CBus::lock(pdMS_TO_TICKS(2))) return;
                _mcp->digitalWrite(_clutchPin, HIGH);//LOW);
                I2CBus::unlock();
            }
            power = false;
            delay(10);
        }
    };
};
