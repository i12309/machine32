#include "ScGuillotine.h"


namespace scene {

ScGuillotine::ScGuillotine(IMachineContext& machine, ScGuard& guard)
    : ScBase(machine, guard) {}

bool ScGuillotine::work(Catalog::DIR direction, uint32_t delayMs, Catalog::SPEED speed, DeviceError::Kind kind, bool withThrow) {
    // Рабочий сценарий гильотины: запуск мотора и guard.
    if (withThrow) machine_.mcpTrigger.arm(McpTrigger::Id::THROW_FORCE);

    runMotor(machine_.mGuillotine, direction, delayMs, speed);
    machine_.mcpTrigger.arm(McpTrigger::Id::GUILLOTINE_HOME);

    guard_.run(machine_.mGuillotine, Catalog::ErrorCode::GUILLOTINE_NOT_IN, kind);
    return true;
}

bool ScGuillotine::stop(Catalog::StopMode mode) {
    // В режиме NotStop проверяем guard и при нужде forceStop.
    if (mode == Catalog::StopMode::NotStop && guard_.check(machine_.mGuillotine)) machine_.mGuillotine->forceStop();

    if (!stopMotor(mode, {machine_.mGuillotine}, {McpTrigger::Id::GUILLOTINE_HOME, McpTrigger::Id::THROW_FORCE})) return false;

    return true;
}

}  // namespace scene
