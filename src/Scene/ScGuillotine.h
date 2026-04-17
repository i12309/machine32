#pragma once

#include "ScBase.h"
#include "Service/DeviceError.h"

namespace scene {

// Сценарии работы узла гильотины.
class ScGuillotine : public ScBase {
public:
    // Создает сценарий гильотины.
    ScGuillotine(IMachineContext& machine, ScGuard& guard);

    // Запускает рабочий ход гильотины и guard.
    bool work(Catalog::DIR direction, uint32_t delayMs = 0, Catalog::SPEED speed = Catalog::SPEED::Normal, DeviceError::Kind kind = DeviceError::Kind::Fatal, bool withThrow = true);

    // Останавливает гильотину и завершает watchdog.
    bool stop(Catalog::StopMode mode);
};

}  // namespace scene

