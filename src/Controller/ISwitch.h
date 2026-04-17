#pragma once

#include "I2CBus.h"
#include "MCP.h"

class ISwitch {
    int _pin;
    int _sigOn;
    MCP* _mcp;
    bool _enabled = true;

    // т.к. у switch сигнал для ег овключения может быть как 0 так и 1 то тут мы понимаем логически как именно выключить. 
    int offLevel() const {
        return (_sigOn == HIGH) ? LOW : HIGH;
    }

    // установить нужный уровень 
    bool writeLevel(int level) {
        if (_mcp == nullptr) {
            digitalWrite(_pin, level);
            return true;
        }
        if (!I2CBus::lock(pdMS_TO_TICKS(2))) return false;
        _mcp->digitalWrite(_pin, level);
        I2CBus::unlock();
        return true;
    }

public:
    bool power = false;

    ISwitch(int pin, int sigOn = HIGH, MCP* mcp = nullptr, bool enabled = true)
        : _pin(pin)
        , _sigOn((sigOn == LOW) ? LOW : HIGH)
        , _mcp(mcp)
        , _enabled(enabled)
    {
        if (!_enabled) return;

        if (_mcp == nullptr) {
            pinMode(_pin, OUTPUT);
        } else if (I2CBus::lock()) {
            _mcp->pinMode(_pin, OUTPUT);
            I2CBus::unlock();
        } else {
            return;
        }

        // В состоянии по умолчанию ключ выключен.
        writeLevel(offLevel());
    }

    // Ключевое слово switch нельзя использовать как имя метода в C++.
    void switch_s() {
        if (power) off();
        else on();
    }

    void on() {
        if (!_enabled || power) return;
        if (!writeLevel(_sigOn)) return;
        power = true;
        delay(2);
    }

    void off() {
        if (!_enabled || !power) return;
        if (!writeLevel(offLevel())) return;
        power = false;
        delay(2);
    }
};
