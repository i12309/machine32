#pragma once

#include "Registry.h"

class McpTrigger {
public:
    enum class Id : uint8_t {
        TABLE_HOME = 0,
        TABLE_UP_LIMIT = 1,
        GUILLOTINE_HOME = 2,
        THROW_FORCE = 3
    };

    static McpTrigger& getInstance() {
        static McpTrigger instance;
        return instance;
    }

    void init();
    bool arm(Id id);
    void disarm(Id id);
    void disarmAll();
    bool isArmed(Id id) const;
    static void IRAM_ATTR mcp_isr(void* arg);
private:
    struct Entry {
        Id id;                    // Логический идентификатор триггера.
        uint8_t bit;              // Бит в маске TaskNotify.
        const char* name;         // Имя для логов.
        const char* sensorName;   // Имя датчика в Registry.
        int expectedSignal;       // Какой уровень считаем валидным срабатыванием.
        bool armed;               // Взведен ли триггер в данный момент.
        MCP* mcp;                 // Разрешенный MCP после resolveEntry().
        int sensorPin;            // Номер пина на MCP после resolveEntry().
    };

    static constexpr uint8_t ENTRY_COUNT = 4;

    Entry entries[ENTRY_COUNT] = {
        {Id::TABLE_HOME, 0, "TABLE_HOME", "TABLE_DOWN", HIGH, false, nullptr, -1},
        {Id::TABLE_UP_LIMIT, 1, "TABLE_UP_LIMIT", "TABLE_UP", HIGH, false, nullptr, -1},
        {Id::GUILLOTINE_HOME, 2, "GUILLOTINE_HOME", "GUILLOTINE", HIGH, false, nullptr, -1},
        {Id::THROW_FORCE, 3, "THROW_FORCE", "THROW", HIGH, false, nullptr, -1},
    };

    TaskHandle_t taskHandle = nullptr; // RTOS-задача, куда ISR складывает события.
    bool initialized = false;          // Защита от повторного init.

    static McpTrigger* isrOwner;       // Singleton-владелец для статического ISR.

    McpTrigger() = default;

    Entry* findEntry(Id id);
    const Entry* findEntry(Id id) const;
    bool resolveEntry(Entry& entry);
    // Возвращает таймаут ожидания I2C для триггерной подсистемы из настроек.
    TickType_t i2cWaitTicks() const;
    // Собирает подмножество битов, относящихся к одному MCP.
    // Нужно для повторной постановки событий, если snapshot не удалось прочитать в этом цикле.
    uint32_t collectEntryBitsForMcp(MCP* mcp, uint32_t activeBits) const;


    static void taskMain(void* pvParameters);

    void processEvents(uint32_t bits);
    void onTriggered(Id id);
};
