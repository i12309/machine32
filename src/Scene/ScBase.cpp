#include "ScBase.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace scene {

ScBase::ScBase(IMachineContext& machine, ScGuard& guard)
    : machine_(machine)
    , guard_(guard) {}

IOptical* ScBase::getOptical(Catalog::OpticalSensor sensor) const {
    // Централизованный выбор оптического датчика.
    switch (sensor) {
        case Catalog::OpticalSensor::MARK:
            return machine_.oMark;
        case Catalog::OpticalSensor::EDGE:
        default:
            return machine_.oEdge;
    }
}

void ScBase::runMotor(IStepper* motor, Catalog::DIR direction, uint32_t delayMs, Catalog::SPEED speed) {
    // Унифицированный запуск мотора для всех сцен.
    if (motor == nullptr) return;
    if (speed != Catalog::SPEED::Custom) motor->setSpeed(speed);

    switch (direction) {
        case Catalog::DIR::Forward:
            motor->runForward();
            break;
        case Catalog::DIR::Backward:
            motor->runBackward();
            break;
    }

    if (delayMs > 0) vTaskDelay(pdMS_TO_TICKS(delayMs));
}

bool ScBase::stopMotor(Catalog::StopMode mode, std::initializer_list<IStepper*> motors, std::initializer_list<McpTrigger::Id> triggers) {
    // Сначала отправляем команду остановки всем моторам.
    for (IStepper* motor : motors) {
        if (motor == nullptr) continue;
        switch (mode) {
            case Catalog::StopMode::ForceStop:
                motor->forceStop();
                break;
            case Catalog::StopMode::SoftStop:
                motor->stopMove();
                break;
            case Catalog::StopMode::NotStop:
                break;
        }
    }

    // Пока хотя бы один мотор движется, остановка не завершена.
    for (IStepper* motor : motors) {
        if (motor != nullptr && motor->isRunning()) return false;
    }

    // Снимаем аппаратные триггеры только после фактической остановки.
    for (McpTrigger::Id trigger : triggers) {
        machine_.mcpTrigger.disarm(trigger);
    }

    // Нормализуем скорость мотора и состояние guard.
    for (IStepper* motor : motors) {
        if (motor == nullptr) continue;
        motor->setSpeed();
        guard_.stop(motor);
    }

    return true;
}

void ScBase::setPaperClutch(bool engage) {
    // Переключаем муфту с короткой безопасной паузой.
    if (!machine_.mPaper->isRunning()) vTaskDelay(pdMS_TO_TICKS(2));

    if (engage) machine_.swCatch->on();
    else machine_.swCatch->off();
}

void ScBase::setThrowSwitch(bool engage) {
    // Переключаем THROW с короткой безопасной паузой.
    if (!machine_.mPaper->isRunning()) vTaskDelay(pdMS_TO_TICKS(2));

    if (engage) machine_.swThrow->on();
    else machine_.swThrow->off();
}

}  // namespace scene
