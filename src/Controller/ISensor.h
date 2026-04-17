#pragma once

#include "I2CBus.h"
#include "MCP.h"

class ISensor {
private:
    int _sensorPin;
    int _typePin; // INPUT/OUTPUT/INPUT_PULLUP/INPUT_PULLDOWN
    MCP* _mcp;
    bool _enabled = true;
    // Кэш последнего успешно прочитанного уровня с MCP.
    // Нужен как безопасный fallback, когда polling временно запрещен или I2C занята.
    bool _hasCachedLevel = false;
    int _cachedLevel = LOW;

public:
    // Инициализирует локальный GPIO либо пин на MCP.
    // Для MCP сразу запоминаем стартовый уровень, чтобы дальше можно было читать из кэша.
    ISensor(int sensorPin, int typePin, MCP* mcp = nullptr, bool enabled = true)
        : _sensorPin(sensorPin)
        , _typePin(typePin)
        , _mcp(mcp)
        , _enabled(enabled)
    {
        if (!_enabled) return;
        if (_mcp == nullptr) {
            pinMode(_sensorPin, _typePin);
            return;
        }

        if (I2CBus::lock()) {
            _mcp->pinMode(_sensorPin, _typePin);
            // Первый снимок сохраняем сразу после настройки пина,
            // чтобы при блокировке polling не возвращать "слепое" значение.
            _cachedLevel = _mcp->digitalRead(_sensorPin, LOW);
            _hasCachedLevel = true;
            I2CBus::unlock();
        }
    }

    // Проверяет ожидаемый уровень на датчике.
    // Для MCP сначала уважаем запрет polling от McpTrigger, затем при возможности обновляем кэш.
    bool check(int sig) {
        if (!_enabled) return false;
        if (_mcp == nullptr) return digitalRead(_sensorPin) == sig;

        // Пока банк удерживается McpTrigger, не читаем GPIO повторно:
        // лишний polling может очистить latch MCP и сорвать обработку прерывания.
        if (_mcp->isPollingBlockedForPin(static_cast<uint8_t>(_sensorPin))) {
            return _hasCachedLevel ? (_cachedLevel == sig) : false;
        }

        delay(2); // Сохраняем прежнюю временную модель чтения через MCP.
        if (!I2CBus::lock(pdMS_TO_TICKS(2))) {
            return _hasCachedLevel ? (_cachedLevel == sig) : false;
        }

        // После успешного чтения обновляем кэш, чтобы следующие fallback-проверки были актуальны.
        _cachedLevel = _mcp->digitalRead(_sensorPin, !sig);
        _hasCachedLevel = true;
        I2CBus::unlock();
        return _cachedLevel == sig;
    }

    int pin() const { return _sensorPin; }
    MCP* mcp() const { return _mcp; }
};
