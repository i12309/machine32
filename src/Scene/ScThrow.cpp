#include "ScThrow.h"

namespace scene {

ScThrow::ScThrow(IMachineContext& machine, ScGuard& guard)
    : ScBase(machine, guard) {}

bool ScThrow::work(Catalog::DIR direction, Catalog::SPEED speed) {
    // Запускаем THROW и крутим общий мотор в нужном направлении.
    setThrowSwitch(true);
    runMotor(machine_.mPaper, direction, 0, speed);
    return true;
}

bool ScThrow::stop(Catalog::StopMode mode) {
    // Остановка THROW после фактической остановки мотора.
    if (!stopMotor(mode, {machine_.mPaper})) return false;
    setThrowSwitch(false);
    //TODO - тут только выключается Enable драйвера но сам мотор не останавливается. Надо найти сценарий когда нужна именно остановка мотора а не просто выключение драйвера
    return true;
}

}  // namespace scene
