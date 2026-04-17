#pragma once

#include "ScBase.h"

namespace scene {

// Сценарии THROW как отдельного режима работы.
// Даже при общем моторе логика THROW хранится отдельно.
class ScThrow : public ScBase {
public:
    // Создает сценарий THROW.
    ScThrow(IMachineContext& machine, ScGuard& guard);

    // Запускает движение в режиме THROW.
    bool work(Catalog::DIR direction, Catalog::SPEED speed = Catalog::SPEED::Slow);

    // Останавливает режим THROW.
    bool stop(Catalog::StopMode mode = Catalog::StopMode::SoftStop);
};

}  // namespace scene

