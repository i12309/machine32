#pragma once

#include <map>

#include "Catalog.h"
#include "Machine/Context/IMachineContext.h"
#include "Service/DeviceError.h"

namespace scene {

// Guard отслеживает лимиты шагов и времени для мотора.
// Используется сценами для watchdog-поведения и диагностики.
class ScGuard {
public:
    // Полный запуск guard: reset, start и активация watchdog.
    // Для TABLE/GUILLOTINE автоматически включает timeout-trigger.
    bool run(IStepper* motor, Catalog::ErrorCode errorCode, DeviceError::Kind kind = DeviceError::Kind::Fatal);

    // Сбрасывает контекст контроля для выбранного мотора.
    void reset(IStepper* motor);

    // Останавливает guard: reset + деактивация watchdog.
    void stop(IStepper* motor);

    // Сбрасывает контексты контроля для всех моторов.
    void resetAll();

    // Запускает контроль лимитов для мотора.
    // Возвращает false, если старт guard невозможен.
    bool start(IStepper* motor, Catalog::ErrorCode errorCode, DeviceError::Kind kind = DeviceError::Kind::Fatal);

    // Проверяет срабатывание лимитов и пишет диагностику.
    bool check(IStepper* motor);

private:
    // Служебный контекст одного отслеживаемого мотора.
    struct GuardContext {
        bool active = false;
        Catalog::ErrorCode errorCode = Catalog::ErrorCode::NO_ERROR;
        DeviceError::Kind kind = DeviceError::Kind::Fatal;
        uint32_t startMs = 0;
        int32_t startPosition = 0;
    };

    // Возвращает имя мотора как ключ для хранения состояния.
    String motorName(IStepper* motor) const;

    // Отправляет запись в DeviceError с нужным уровнем.
    void report(const GuardContext& ctx, const String& shortDetails) const;

    // Включает watchdog-trigger для TABLE/GUILLOTINE.
    void activateWatchdogFor(IStepper* motor) const;

    // Выключает watchdog-trigger для TABLE/GUILLOTINE.
    void deactivateWatchdogFor(IStepper* motor) const;

    std::map<String, GuardContext> states_;
};

}  // namespace scene
