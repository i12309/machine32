#pragma once

#include "ScBase.h"

namespace scene {

// Сценарии подъема и опускания стола.
class ScTable : public ScBase {
public:
    // Создает сценарий стола.
    ScTable(IMachineContext& machine, ScGuard& guard);

    // Поднимает стол до верхнего лимита.
    Catalog::TableActionResult up(Catalog::SPEED speed = Catalog::SPEED::Normal, bool blocking = false);

    // Опускает стол до домашнего датчика.
    Catalog::TableActionResult down(Catalog::SPEED speed = Catalog::SPEED::Normal);

    // Останавливает движение стола с проверкой guard.
    bool stop(Catalog::StopMode mode = Catalog::StopMode::ForceStop);
};

}  // namespace scene

