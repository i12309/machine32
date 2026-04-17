#pragma once

#include <initializer_list>

#include "Catalog.h"
#include "Controller/McpTrigger.h"
#include "Machine/Context/IMachineContext.h"
#include "ScGuard.h"

namespace scene {

// Базовый класс для общих операций всех сцен.
// Содержит переиспользуемые helper-методы для моторов и переключателей.
class ScBase {
public:
    // Создает базу и сохраняет ссылки на контекст машины и guard.
    ScBase(IMachineContext& machine, ScGuard& guard);
    virtual ~ScBase() = default;

protected:
    // Возвращает нужный оптический датчик по типу.
    IOptical* getOptical(Catalog::OpticalSensor sensor) const;

    // Запускает мотор в нужном направлении и скорости.
    void runMotor(IStepper* motor, Catalog::DIR direction, uint32_t delayMs = 0, Catalog::SPEED speed = Catalog::SPEED::Normal);

    // Останавливает группу моторов и очищает связанные триггеры.
    bool stopMotor(Catalog::StopMode mode, std::initializer_list<IStepper*> motors, std::initializer_list<McpTrigger::Id> triggers = {});

    // Управляет муфтой захвата бумаги.
    void setPaperClutch(bool engage);

    // Управляет включением линии THROW.
    void setThrowSwitch(bool engage);

    IMachineContext& machine_;
    ScGuard& guard_;
};

}  // namespace scene

