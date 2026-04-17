#include "McpTrigger.h"

#include "I2CBus.h"
#include "Machine/Machine.h"
#include "Service/Log.h"

McpTrigger* McpTrigger::isrOwner = nullptr;

// Инициализация подсистемы триггеров:
// - сохраняем владельца ISR (singleton),
// - поднимаем отдельную задачу обработки событий,
// - сбрасываем все триггеры в безопасное состояние.
void McpTrigger::init() {
    if (initialized) return;

    initialized = true;
    isrOwner = this;

    if (taskHandle == nullptr) {
        xTaskCreatePinnedToCore(
            McpTrigger::taskMain,
            "McpTrigger",
            4096,
            this,
            configMAX_PRIORITIES - 2,
            &taskHandle,
            1
        );
    }

    disarmAll();
}

McpTrigger::Entry* McpTrigger::findEntry(Id id) {
    for (auto& entry : entries) {
        if (entry.id == id) return &entry;
    }
    return nullptr;
}

const McpTrigger::Entry* McpTrigger::findEntry(Id id) const {
    for (const auto& entry : entries) {
        if (entry.id == id) return &entry;
    }
    return nullptr;
}

// Связывает логический триггер с реальным сенсором из Registry.
// На выходе Entry знает, с каким MCP и каким пином работать.
bool McpTrigger::resolveEntry(Entry& entry) {
    ISensor* sensor = Registry::getInstance().getSensor(entry.sensorName);
    if (sensor == nullptr) {
        Log::L("[McpTrigger] resolve fail trigger=%s: sensor is null", entry.name);
        return false;
    }

    MCP* mcp = sensor->mcp();
    if (mcp == nullptr) {
        Log::L("[McpTrigger] resolve fail trigger=%s: sensor has no MCP", entry.name);
        return false;
    }

    int pin = sensor->pin();
    if (pin < 0 || pin > 15) {
        Log::L("[McpTrigger] resolve fail trigger=%s: invalid sensor pin=%d", entry.name, pin);
        return false;
    }

    int intPin = mcp->getIntPinFor(static_cast<uint8_t>(pin));
    if (intPin == -1) {
        Log::L("[McpTrigger] resolve fail trigger=%s: MCP int pin not configured", entry.name);
        return false;
    }

    entry.mcp = mcp;
    entry.sensorPin = pin;
    return true;
}

TickType_t McpTrigger::i2cWaitTicks() const {
    // Настройка позволяет подстроить поведение под реальную загрузку I2C.
    // Если значение в конфиге некорректное, откатываемся к безопасным 150 мс.
    const int waitMs = Core::settings.MCP_TRIGGER_I2C_WAIT_MS;
    return pdMS_TO_TICKS((waitMs > 0) ? waitMs : 150);
}

uint32_t McpTrigger::collectEntryBitsForMcp(MCP* mcp, uint32_t activeBits) const {
    if (mcp == nullptr) return 0;

    uint32_t result = 0;
    for (const auto& entry : entries) {
        if ((activeBits & (1UL << entry.bit)) == 0) continue;
        if (!entry.armed) continue;
        if (entry.mcp != mcp) continue;
        result |= (1UL << entry.bit);
    }
    return result;
}

void IRAM_ATTR McpTrigger::mcp_isr(void* arg) {
    auto* entry = static_cast<Entry*>(arg);
    auto* self = isrOwner;
    if (self == nullptr || entry == nullptr || !entry->armed || self->taskHandle == nullptr) return;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(self->taskHandle, (1UL << entry->bit), eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Отдельная RTOS-задача:
// получает битовую маску сработавших триггеров и обрабатывает их вне ISR.
void McpTrigger::taskMain(void* pvParameters) {
    auto* self = static_cast<McpTrigger*>(pvParameters);
    if (self == nullptr) return;

    uint32_t bits = 0;
    for (;;) {
        xTaskNotifyWait(0, 0xFFFFFFFFUL, &bits, portMAX_DELAY);
        self->processEvents(bits);
    }
}

// Взвод триггера:
// - проверка/разрешение источника (сенсор -> MCP -> пин),
// - настройка MCP interrupt-настроек для этого пина,
// - подключение ISR-маршрута через MCP::attachInterruptHandler.
bool McpTrigger::arm(Id id) {
    if (!initialized) init();

    Entry* entry = findEntry(id);
    if (entry == nullptr) return false;

    if (!resolveEntry(*entry)) {
        // Если сенсор больше не разрешается в MCP/pin, оставляем триггер в снятом состоянии.
        entry->armed = false;
        return false;
    }

    if (entry->armed) disarm(id);

    // С этого момента polling по банку блокируем:
    // пока триггер взведен, чтения "в лоб" через ISensor/IEncoder не должны трогать этот MCP-банк.
    entry->mcp->protectBankForPin(static_cast<uint8_t>(entry->sensorPin));

    if (!I2CBus::lock(i2cWaitTicks())) {
        entry->mcp->unprotectBankForPin(static_cast<uint8_t>(entry->sensorPin));
        Log::L("[McpTrigger] arm %s fail: I2C busy", entry->name);
        return false;
    }
    // Используем CHANGE вместо RISING/FALLING:
    // RISING/FALLING в MCP23017 работают через DEFVAL (уровневое сравнение),
    // и если пин уже в ожидаемом состоянии при arm(), MCP мгновенно защёлкивает
    // прерывание — INT уходит в LOW до подключения ISR, и дальше фронтов нет.
    // CHANGE сравнивает с последним прочитанным значением GPIO, а processEvents()
    // уже фильтрует по expectedSignal, обеспечивая детекцию нужного фронта.
    bool ok = entry->mcp->setupInterrupts(false, false, LOW) &&
              entry->mcp->clearInterrupts() &&
              entry->mcp->setupInterruptPin(static_cast<uint8_t>(entry->sensorPin), CHANGE);
    // Чтение GPIO устанавливает "предыдущее значение" для CHANGE-детекции
    // равным текущему состоянию пина и сбрасывает возможное мгновенное прерывание.
    if (ok) {
        entry->mcp->digitalRead(static_cast<uint8_t>(entry->sensorPin));
    }
    I2CBus::unlock();
    if (!ok) {
        entry->mcp->unprotectBankForPin(static_cast<uint8_t>(entry->sensorPin));
        Log::L("[McpTrigger] arm %s fail: MCP offline", entry->name);
        return false;
    }

    if (!entry->mcp->attachInterruptHandler(static_cast<uint8_t>(entry->sensorPin), entry, FALLING)) {
        // Host GPIO для INTA/INTB не удалось подвязать.
        // Откатываем и конфигурацию MCP, и защиту банка, чтобы не оставить систему в полувзведенном состоянии.
        if (I2CBus::lock(i2cWaitTicks())) {
            entry->mcp->disableInterruptPin(static_cast<uint8_t>(entry->sensorPin));
            entry->mcp->clearInterrupts();
            I2CBus::unlock();
        }
        entry->mcp->unprotectBankForPin(static_cast<uint8_t>(entry->sensorPin));
        Log::L("[McpTrigger] arm %s fail: host IRQ attach failed", entry->name);
        return false;
    }

    entry->armed = true;

    Log::L("[McpTrigger] arm %s: OK (pin=%d mode=CHANGE)", entry->name, entry->sensorPin);
    return true;
}

// Снятие триггера:
// - отключаем GPIO ISR-роутинг для пина,
// - выключаем interrupt по пину в MCP,
// - очищаем latch MCP interrupt-регистров.
void McpTrigger::disarm(Id id) {
    if (!initialized) return;

    Entry* entry = findEntry(id);
    if (entry == nullptr) return;

    if (entry->mcp != nullptr && entry->sensorPin >= 0) {
        entry->mcp->detachInterruptHandler(static_cast<uint8_t>(entry->sensorPin));
        if (I2CBus::lock(i2cWaitTicks())) {
            entry->mcp->disableInterruptPin(static_cast<uint8_t>(entry->sensorPin));
            // entry->mcp->clearInterrupts(); !!!!
            I2CBus::unlock();
        }
        // После полного disarm polling этого банка снова разрешен.
        entry->mcp->unprotectBankForPin(static_cast<uint8_t>(entry->sensorPin));
    }

    entry->armed = false;
}

void McpTrigger::disarmAll() {
    if (!initialized) {
        for (auto& entry : entries) entry.armed = false;
        return;
    }

    for (auto& entry : entries) {
        disarm(entry.id);
    }
}

bool McpTrigger::isArmed(Id id) const {
    const Entry* entry = findEntry(id);
    return entry != nullptr && entry->armed;
}

// Главная логика разбора IRQ:
// используем "снимок" регистров MCP (INTF + INTCAP), а не getLastInterruptPin().
// Это нужно, чтобы не терять события, если несколько пинов сработали почти одновременно.
void McpTrigger::processEvents(uint32_t bits) {
    // Снимок одного MCP:
    // flags    - какие пины подняли interrupt (INTF),
    // captured - уровни этих пинов в момент latch interrupt (INTCAP).
    struct Snapshot {
        MCP* mcp = nullptr;
        uint16_t flags = 0;
        uint16_t captured = 0;
        bool valid = false;
    };

    Snapshot snapshots[ENTRY_COUNT];
    uint8_t snapshotCount = 0;
    // Биты, которые не удалось разобрать сейчас из-за занятой I2C или невалидного snapshot.
    // Их вернем обратно в очередь задачи коротким retry.
    uint32_t retryBits = 0;

    // Ищем/создаём слот снимка для конкретного MCP.
    auto findSnapshot = [&](MCP* mcp) -> Snapshot* {
        for (uint8_t i = 0; i < snapshotCount; ++i) {
            if (snapshots[i].mcp == mcp) return &snapshots[i];
        }
        if (snapshotCount >= ENTRY_COUNT) return nullptr;
        snapshots[snapshotCount].mcp = mcp;
        return &snapshots[snapshotCount++];
    };

    // 1) Собираем уникальные MCP, по которым пришли IRQ-биты.
    for (auto& entry : entries) {
        if ((bits & (1UL << entry.bit)) == 0) continue;
        if (!entry.armed) continue;
        if (entry.mcp == nullptr || entry.sensorPin < 0 || entry.sensorPin > 15) continue;
        findSnapshot(entry.mcp);
    }

    // 2) Один раз читаем snapshot для каждого MCP под I2C mutex.
    for (uint8_t i = 0; i < snapshotCount; ++i) {
        if (!I2CBus::lock(i2cWaitTicks())) {
            // Нельзя терять уже пришедший IRQ только потому, что шина занята в этот момент.
            retryBits |= collectEntryBitsForMcp(snapshots[i].mcp, bits);
            continue;
        }
        snapshots[i].valid = snapshots[i].mcp->readInterruptSnapshot(
            snapshots[i].flags,
            snapshots[i].captured
        );
        I2CBus::unlock();
        if (!snapshots[i].valid) {
            // MCP не отдал корректный снимок: повторим обработку позже теми же битами.
            retryBits |= collectEntryBitsForMcp(snapshots[i].mcp, bits);
        }
    }

    // 3) Вычисляем, какие логические триггеры реально сработали в этом цикле.
    bool shouldFire[ENTRY_COUNT] = {false};
    bool needClear[ENTRY_COUNT] = {false};
    for (uint8_t idx = 0; idx < ENTRY_COUNT; ++idx) {
        auto& entry = entries[idx];
        if ((bits & (1UL << entry.bit)) == 0) continue;
        if (!entry.armed) continue;
        if (entry.mcp == nullptr || entry.sensorPin < 0 || entry.sensorPin > 15) continue;

        Snapshot* snap = nullptr;
        int8_t snapIndex = -1;
        for (uint8_t i = 0; i < snapshotCount; ++i) {
            if (snapshots[i].mcp == entry.mcp) {
                snap = &snapshots[i];
                snapIndex = static_cast<int8_t>(i);
                break;
            }
        }
        if (snap == nullptr || !snap->valid) continue;

        const uint16_t pinMask = static_cast<uint16_t>(1U << static_cast<uint8_t>(entry.sensorPin));
        if ((snap->flags & pinMask) == 0) continue;

        // Важный момент:
        // проверяем не "текущее" состояние пина, а зафиксированный INTCAP-уровень.
        const int capturedLevel = ((snap->captured & pinMask) != 0) ? HIGH : LOW;
        if (entry.expectedSignal != -1 && capturedLevel != entry.expectedSignal) {
            // Safety fallback: if INTCAP mismatch happened due bounce/race,
            // read current GPIO level and accept it when it already matches expectedSignal.
            bool recoveredByLiveLevel = false;
            if (I2CBus::lock(i2cWaitTicks())) {
                const int liveLevel = entry.mcp->digitalRead(static_cast<uint8_t>(entry.sensorPin), capturedLevel);
                I2CBus::unlock();
                recoveredByLiveLevel = (liveLevel == entry.expectedSignal);
            }
            if (!recoveredByLiveLevel) {
                if (snapIndex >= 0) needClear[static_cast<uint8_t>(snapIndex)] = true;
                continue;
            }
        }
        shouldFire[idx] = true;
    }

    // 4) Реакцию выполняем после завершения разбора snapshot:
    // сначала disarm (одноразовость), затем бизнес-логика onTriggered.
    for (uint8_t idx = 0; idx < ENTRY_COUNT; ++idx) {
        if (!shouldFire[idx]) continue;
        disarm(entries[idx].id);
        onTriggered(entries[idx].id);
    }

    // Если были "чужие" уровни по ожидаемому сигналу, освобождаем IRQ-линию MCP.
    for (uint8_t i = 0; i < snapshotCount; ++i) {
        if (!needClear[i] || !snapshots[i].valid) continue;
        if (I2CBus::lock(i2cWaitTicks())) {
            snapshots[i].mcp->clearInterrupts();
            I2CBus::unlock();
        }
    }

    if (retryBits != 0 && taskHandle != nullptr) {
        // Даем шине один тик освободиться и повторно ставим событие в ту же задачу.
        // Это дешевле, чем держать длительный busy-wait внутри processEvents().
        vTaskDelay(pdMS_TO_TICKS(1));
        xTaskNotify(taskHandle, retryBits, eSetBits);
    }

}

// Прикладная реакция на сработавший триггер.
// Здесь только "тяжёлая" логика, в ISR она не выполняется.
void McpTrigger::onTriggered(Id id) {
    IMachineContext& ctx = Machine::getInstance().context();
    switch (id) {
        case Id::TABLE_HOME: {
            ctx.mTable->forceStop();
            ctx.mTable->setCurrentPosition(0);
            break;
        }
        case Id::TABLE_UP_LIMIT: {
            ctx.mTable->forceStop();
            break;
        }
        case Id::GUILLOTINE_HOME: {
            ctx.mGuillotine->forceStop();
            break;
        }
        case Id::THROW_FORCE: {
            const bool catchWasOn = ctx.swCatch->power;
            const bool paperWasOn = ctx.swPaper->power;
            const bool throwWasOn = ctx.swThrow->power;

            //if (catchWasOn) 
            ctx.swCatch->off();
            //if (paperWasOn) 
            ctx.swPaper->off();
            if (!throwWasOn) ctx.swThrow->on();

            ctx.mPaper->setSpeed(Catalog::SPEED::Force);
            int32_t forceSteps = ctx.mPaper->getWorkMove();
            //if (forceSteps <= 0) forceSteps = 3200;
            ctx.mPaper->move(forceSteps, true);
            ctx.mPaper->setSpeed();

            if (catchWasOn) ctx.swCatch->on();
            if (paperWasOn) ctx.swPaper->on();
            if (!throwWasOn) ctx.swThrow->off();
            break;
        }
    }
}
