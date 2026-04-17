#include "ScTable.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


namespace scene {

ScTable::ScTable(IMachineContext& machine, ScGuard& guard)
    : ScBase(machine, guard) {}

Catalog::TableActionResult ScTable::up(Catalog::SPEED speed, bool blocking) {
    // Сценарий подъема стола.
    machine_.mcpTrigger.disarm(McpTrigger::Id::TABLE_HOME);
    if (machine_.sTableUp->check(HIGH)) return Catalog::TableActionResult::AtLimit;

    // Перед запуском обязательно взводим верхний лимитный триггер.
    if (!machine_.mcpTrigger.arm(McpTrigger::Id::TABLE_UP_LIMIT)) {
        return Catalog::TableActionResult::TriggerFault;
    }

    runMotor(machine_.mTable, Catalog::DIR::Forward, 0, speed);
    guard_.run(machine_.mTable, Catalog::ErrorCode::TABLE_NOT_UP);

    // В блокирующем режиме ждем полной остановки.
    if (blocking) {
        while (!stop(Catalog::StopMode::NotStop)) vTaskDelay(pdMS_TO_TICKS(5));
    }

    return Catalog::TableActionResult::Started;
}

Catalog::TableActionResult ScTable::down(Catalog::SPEED speed) {
    // Сценарий опускания стола.
    machine_.mcpTrigger.disarm(McpTrigger::Id::TABLE_UP_LIMIT);
    machine_.mcpTrigger.disarm(McpTrigger::Id::TABLE_HOME);

    if (machine_.sTableDown->check(HIGH)) {
        machine_.mTable->setCurrentPosition(0);
        return Catalog::TableActionResult::AtLimit;
    }

    // Перед движением вниз нужен домашний триггер.
    if (!machine_.mcpTrigger.arm(McpTrigger::Id::TABLE_HOME)) {
        return Catalog::TableActionResult::TriggerFault;
    }

    runMotor(machine_.mTable, Catalog::DIR::Backward, 0, speed);

    // Fallback: если датчик уже пойман, останавливаемся сразу.
    if (machine_.sTableDown->check(HIGH)) {
        machine_.mTable->forceStop();
        machine_.mTable->setCurrentPosition(0);
        machine_.mcpTrigger.disarm(McpTrigger::Id::TABLE_HOME);
        return Catalog::TableActionResult::AtLimit;
    }

    guard_.run(machine_.mTable, Catalog::ErrorCode::TABLE_NOT_DOWN);
    return Catalog::TableActionResult::Started;
}

bool ScTable::stop(Catalog::StopMode mode) {
    // В режиме NotStop только проверяем Guard и, при необходимости, аварийно тормозим.
    if (mode == Catalog::StopMode::NotStop && guard_.check(machine_.mTable)) machine_.mTable->forceStop();
    if (!stopMotor(mode, {machine_.mTable}, {McpTrigger::Id::TABLE_HOME, McpTrigger::Id::TABLE_UP_LIMIT})) return false;

    // Если остановились в домашнем датчике, фиксируем нулевую позицию.
    //if (machine_.sTableDown->check(HIGH)) machine_.mTable->setCurrentPosition(0);
    return true;
}

}  // namespace scene
