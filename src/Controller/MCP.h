#pragma once

#include <Adafruit_MCP23X17.h>
#include <array>

#include "I2CBus.h"
#include "Service/Log.h"

class MCP {
private:
    // Обертка над одним MCP23017:
    // хранит метаданные чипа и маршрутизирует прерывания банков в колбэки пинов.
    String name;
    String type;
    uint8_t address;
    uint8_t number;
    Adafruit_MCP23X17* mcp = nullptr;

    // GPIO ESP32, подключенные к линиям MCP INTA/INTB (-1 = не подключено).
    int intPinA = -1;
    int intPinB = -1;

    // Слот обработчика для одного пина внутри банка (0..7).
    struct HandlerSlot {
        void* arg = nullptr;
        bool active = false;
    };

    std::array<HandlerSlot, 8> handlersA{};
    std::array<HandlerSlot, 8> handlersB{};
    bool routerAttachedA = false;
    bool routerAttachedB = false;
    // Счетчик "владельцев" банка A/B.
    // Пока счетчик > 0, обычный polling этого банка запрещен, чтобы не сбивать IRQ-логику.
    volatile uint8_t bankProtectionCount[2] = {0, 0};

    // ISR-маршрутизатор для банка A (пины 0..7).
    static void IRAM_ATTR interruptRouterA(void* arg);

    // ISR-маршрутизатор для банка B (пины 8..15).
    static void IRAM_ATTR interruptRouterB(void* arg);

    // Прерывание банка может быть вызвано любым пином этого банка.
    // Вызываем все активные слоты, а верхний слой уже фильтрует точный источник.
    void IRAM_ATTR dispatchBankISR(uint8_t bank);

    // Есть ли хотя бы один активный подписчик в выбранном банке.
    bool hasActiveHandlers(uint8_t bank) const {
        const auto& handlers = (bank == 0) ? handlersA : handlersB;
        for (const auto& slot : handlers) {
            if (slot.active) return true;
        }
        return false;
    }

    // Чтение одного регистра MCP23017 (BANK=0: A/B идут подряд).
    bool readRegisterRaw(uint8_t reg, uint8_t& value) {
        Wire.beginTransmission(address);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0) return false;
        if (Wire.requestFrom(static_cast<int>(address), 1) != 1) return false;
        value = Wire.read();
        return true;
    }

public:
    // Создаёт объект-обёртку MCP и пытается инициализировать чип на I2C.
    // Если чип недоступен, mcp остаётся nullptr, а безопасные методы ниже вернут false/fallback.
    MCP(String _name, String _type, String _addressStr, int int_pin_a = -1, int int_pin_b = -1) {
        type = _type;
        // Адрес приходит из конфига в hex-строке, например "20", "21".
        address = strtol(_addressStr.c_str(), NULL, 16);// Преобразуем адрес из строки в число
        name = _name;
        // Логический номер, используемый в текущей схеме маппинга внешних пинов.
        number = address - 0x20;
        intPinA = int_pin_a;
        intPinB = int_pin_b;

        mcp = new Adafruit_MCP23X17();
        if (mcp == nullptr) {
            Log::E("MCP23017 '%s' allocation failed.", name.c_str());
            return;
        }

        if (!I2CBus::lock(pdMS_TO_TICKS(20))) {
            delete mcp;
            mcp = nullptr;
            Log::D("MCP23017 '%s' skipped: I2C busy.", name.c_str());
            return;
        }

        Wire.beginTransmission(address);
        const uint8_t probeErr = Wire.endTransmission(true);
        if (probeErr != 0) {
            I2CBus::unlock();
            delete mcp;
            mcp = nullptr;
            Log::D("MCP23017 '%s' не найден на I2C шине.", name.c_str());
            return;
        }

        bool online = mcp->begin_I2C(address, &Wire);
        I2CBus::unlock();

        if (online) {
            Log::L("MCP23017 '%s' (адрес 0x%02X) инициализирован.", name.c_str(), address);
        } else {
            delete mcp;
            mcp = nullptr;
            Log::D("MCP23017 '%s' не найден на I2C шине.", name.c_str());
        }
    }

    // Служебные идентификаторы MCP из конфига.
    String getName() { return name; }
    uint8_t getNumber() { return number; }

    // Прямой доступ к низкоуровневому драйверу для специальных операций.
    Adafruit_MCP23X17* IO() {return mcp;}

    // Безопасные обертки: защищают от nullptr при оффлайн MCP.
    bool pinMode(uint8_t pin, uint8_t mode) {
        if (mcp == nullptr) return false;
        mcp->pinMode(pin, mode);
        return true;
    }

    bool digitalWrite(uint8_t pin, bool value) {
        if (mcp == nullptr) return false;
        mcp->digitalWrite(pin, value);
        return true;
    }

    int digitalRead(uint8_t pin, int fallback = LOW) {
        if (mcp == nullptr) return fallback;
        return mcp->digitalRead(pin);
    }

    bool setupInterrupts(bool mirroring, bool openDrain, uint8_t polarity) {
        if (mcp == nullptr) return false;
        mcp->setupInterrupts(mirroring, openDrain, polarity);
        return true;
    }

    bool clearInterrupts() {
        if (mcp == nullptr) return false;
        mcp->clearInterrupts();
        return true;
    }

    bool setupInterruptPin(uint8_t pin, uint8_t mode) {
        if (mcp == nullptr) return false;
        mcp->setupInterruptPin(pin, mode);
        return true;
    }

    bool disableInterruptPin(uint8_t pin) {
        if (mcp == nullptr) return false;
        mcp->disableInterruptPin(pin);
        return true;
    }

    // Совместимость со старой логикой (single-pin источник).
    // Для новой надёжной логики использовать readInterruptSnapshot().
    uint8_t getLastInterruptPin(uint8_t fallback = 0xFF) {
        if (mcp == nullptr) return fallback;
        return mcp->getLastInterruptPin();
    }

    // Снимок IRQ за один проход: какие пины прервали (INTF) и их уровни в момент IRQ (INTCAP).
    // Требует внешней синхронизации I2C (через I2CBus::lock).
    bool readInterruptSnapshot(uint16_t& flags, uint16_t& captured) {
        if (mcp == nullptr) return false;

        uint8_t intfA = 0;
        uint8_t intfB = 0;
        uint8_t intcapA = 0;
        uint8_t intcapB = 0;

        // MCP23X17, BANK=0:
        // INTFA=0x0E, INTFB=0x0F, INTCAPA=0x10, INTCAPB=0x11
        if (!readRegisterRaw(0x0E, intfA)) return false;
        if (!readRegisterRaw(0x0F, intfB)) return false;
        if (!readRegisterRaw(0x10, intcapA)) return false;
        if (!readRegisterRaw(0x11, intcapB)) return false;

        flags = static_cast<uint16_t>(intfA) | (static_cast<uint16_t>(intfB) << 8);
        captured = static_cast<uint16_t>(intcapA) | (static_cast<uint16_t>(intcapB) << 8);
        return true;
    }

    // Какая линия прерывания ESP32 соответствует этому пину MCP.
    int getIntPinFor(uint8_t pin) {
        return (pin < 8) ? intPinA : intPinB;
    }

    // Помечает банк пина как занятый триггерной логикой.
    // Используется как ref-count, чтобы несколько триггеров на одном банке не снимали защиту преждевременно.
    void protectBankForPin(uint8_t pin) {
        const uint8_t bank = (pin < 8) ? 0 : 1;
        if (bankProtectionCount[bank] < 0xFF) {
            ++bankProtectionCount[bank];
        }
    }

    // Освобождает банк после disarm/rollback.
    void unprotectBankForPin(uint8_t pin) {
        const uint8_t bank = (pin < 8) ? 0 : 1;
        if (bankProtectionCount[bank] > 0) {
            --bankProtectionCount[bank];
        }
    }

    // Проверяет, можно ли безопасно читать этот банк через polling,
    // или сейчас им владеет McpTrigger.
    bool isPollingBlockedForPin(uint8_t pin) const {
        const uint8_t bank = (pin < 8) ? 0 : 1;
        return bankProtectionCount[bank] > 0;
    }

    // Регистрирует обработчик для пина и гарантирует подключение ISR-роутера банка.
    // Возвращает false, если host GPIO для INTA/INTB не настроен и взвод нужно откатить.
    bool attachInterruptHandler(uint8_t pin, void* arg, int mode) {
        uint8_t bank = (pin < 8) ? 0 : 1;
        uint8_t localPin = pin & 0x07;
        int gpio = getIntPinFor(pin);
        if (gpio == -1) {
            //Log::L("[MCP] attachInterruptHandler fail mcp=%s pin=%d: int gpio not configured", name.c_str(), pin);
            return false;
        }

        int irq = digitalPinToInterrupt(gpio);
        if (irq == NOT_AN_INTERRUPT) {
            //Log::L("[MCP] attachInterruptHandler fail mcp=%s pin=%d gpio=%d: NOT_AN_INTERRUPT", name.c_str(), pin, gpio);
            return false;
        }

        auto& handlers = (bank == 0) ? handlersA : handlersB;
        handlers[localPin].arg = arg;
        handlers[localPin].active = true;

        // Важно: настраиваем линию прерывания GPIO ESP32, а не пин MCP.
        ::pinMode(gpio, INPUT);
        if (bank == 0) {
            if (!routerAttachedA) {
                attachInterruptArg(irq, interruptRouterA, this, mode);
                routerAttachedA = true;
            }
        } else {
            if (!routerAttachedB) {
                attachInterruptArg(irq, interruptRouterB, this, mode);
                routerAttachedB = true;
            }
        }

        //Log::D("[MCP] irq attached mcp=%s pin=%d gpio=%d", name.c_str(), pin, gpio);
        return true;
    }

    // Удаляет обработчик пина; если активных больше нет, отключает роутер банка.
    void detachInterruptHandler(uint8_t pin) {
        uint8_t bank = (pin < 8) ? 0 : 1;
        uint8_t localPin = pin & 0x07;
        int gpio = getIntPinFor(pin);
        if (gpio == -1) return;

        int irq = digitalPinToInterrupt(gpio);
        if (irq == NOT_AN_INTERRUPT) return;

        auto& handlers = (bank == 0) ? handlersA : handlersB;
        handlers[localPin].active = false;
        handlers[localPin].arg = nullptr;

        if (!hasActiveHandlers(bank)) {
            if (bank == 0) {
                if (routerAttachedA) {
                    detachInterrupt(irq);
                    routerAttachedA = false;
                }
            } else {
                if (routerAttachedB) {
                    detachInterrupt(irq);
                    routerAttachedB = false;
                }
            }
        }
    }
};
