#include "ScPaper.h"

#include <cstdlib>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


namespace scene {

ScPaper::ScPaper(IMachineContext& machine, ScGuard& guard)
    : ScBase(machine, guard) {}

void ScPaper::disableOpticalTriggers() {
    // Снимаем триггеры обоих оптических датчиков.
    machine_.oMark->disableTrigger();
    machine_.oEdge->disableTrigger();
}

Catalog::PaperActionResult ScPaper::work(Catalog::DIR direction, Catalog::SPEED speed, bool withThrow, bool engageClutch) {
    // Базовый непрерывный сценарий протяжки.
    setPaperClutch(engageClutch);
    setThrowSwitch(withThrow);
    machine_.swPaper->on();
    runMotor(machine_.mPaper, direction, 0, speed);
    return Catalog::PaperActionResult::Started;
}

Catalog::PaperActionResult ScPaper::detectPaper(bool withThrow, Catalog::DIR direction, Catalog::SPEED speed, bool engageClutch, int32_t detectOffset, Catalog::OpticalSensor opticalSensor, Catalog::ErrorCode timeoutErrorCode) {
    // Поиск края бумаги по оптике.
    IOptical* optical = getOptical(opticalSensor);

    if (optical->checkWhite()) {
        guard_.reset(machine_.mPaper);
        return Catalog::PaperActionResult::Finished;
    }

    if (speed == Catalog::SPEED::Custom) {
        machine_.mPaper->setSpeed("Paper");
    } else {
        machine_.mPaper->setSpeed(speed);
    }

    Catalog::PaperActionResult result = Catalog::PaperActionResult::Started;
    if (detectOffset == 0) {
        result = work(direction, speed, withThrow, engageClutch);
    } else {
        result = move(1000000, direction, speed, false, engageClutch, withThrow);
    }

    guard_.run(machine_.mPaper, timeoutErrorCode);

    const int32_t signedOffset = (direction == Catalog::DIR::Backward) ? -static_cast<int32_t>(abs(detectOffset)) : static_cast<int32_t>(abs(detectOffset));
    optical->enableTrigger(FALLING, signedOffset);
    return result;
}

Catalog::PaperActionResult ScPaper::detectMark(bool withThrow, Catalog::DIR direction, Catalog::SPEED speed, bool engageClutch, int32_t detectOffset, Catalog::OpticalSensor opticalSensor, Catalog::ErrorCode timeoutErrorCode) {
    // Поиск метки по оптике.
    IOptical* optical = getOptical(opticalSensor);

    if (optical->checkBlack()) {
        guard_.reset(machine_.mPaper);
        return Catalog::PaperActionResult::Finished;
    }

    if (speed == Catalog::SPEED::Custom) {
        machine_.mPaper->setSpeed("Mark");
    } else {
        machine_.mPaper->setSpeed(speed);
    }

    Catalog::PaperActionResult result = Catalog::PaperActionResult::Started;
    if (detectOffset == 0) {
        result = work(direction, speed, withThrow, engageClutch);
    } else {
        result = move(1000000, direction, speed, false, engageClutch, withThrow);
    }

    guard_.run(machine_.mPaper, timeoutErrorCode);

    const int32_t signedOffset = (direction == Catalog::DIR::Backward) ? -static_cast<int32_t>(abs(detectOffset)) : static_cast<int32_t>(abs(detectOffset));
    optical->enableTrigger(RISING, signedOffset);
    return result;
}

Catalog::PaperActionResult ScPaper::move(int32_t steps, Catalog::DIR direction, Catalog::SPEED speed, bool blocking, bool engageClutch, bool withThrow) {
    // Перемещаем бумагу на фиксированную дистанцию.
    setPaperClutch(engageClutch);
    setThrowSwitch(withThrow);
    machine_.swPaper->on();
    if (speed != Catalog::SPEED::Custom) machine_.mPaper->setSpeed(speed);

    const int32_t signedSteps = (direction == Catalog::DIR::Backward) ? -steps : steps;
    machine_.mPaper->move(signedSteps, false);

    if (blocking) {
        while (!stop(Catalog::StopMode::NotStop)) vTaskDelay(pdMS_TO_TICKS(5));
        return Catalog::PaperActionResult::Finished;
    }

    return Catalog::PaperActionResult::Started;
}

bool ScPaper::stopOffset(int32_t offsetSteps) {
    // Остановка после срабатывания оптики с offset.
    if (offsetSteps == 0) return stop(Catalog::StopMode::ForceStop);

    disableOpticalTriggers();
    const int32_t target = machine_.mPaper->getCurrentPosition() + offsetSteps;

    const int64_t offsetAbs = llabs(static_cast<long long>(offsetSteps));
    const int64_t paperStopPath = llabs(static_cast<long long>(machine_.mPaper->stepsToStop()));

    if (paperStopPath > offsetAbs) {
        // Тормозной путь слишком длинный, поэтому сначала forceStop.
        machine_.mPaper->forceStop();
        const int32_t correction = target - machine_.mPaper->getCurrentPosition();
        if (correction != 0) machine_.mPaper->move(correction, false);
    } else {
        // Есть запас по тормозному пути, используем moveTo(target).
        machine_.mPaper->moveTo(target, false);
    }

    while (!stop(Catalog::StopMode::NotStop)) vTaskDelay(pdMS_TO_TICKS(5));
    return true;
}

bool ScPaper::stop(Catalog::StopMode mode) {
    // Остановка сценария бумаги с cleanup связанных ресурсов.
    switch (mode) {
        case Catalog::StopMode::ForceStop:
            machine_.mPaper->forceStop();
            guard_.reset(machine_.mPaper);
            disableOpticalTriggers();
            break;

        case Catalog::StopMode::SoftStop:
            machine_.mPaper->stopMove();
            guard_.reset(machine_.mPaper);
            disableOpticalTriggers();
            break;

        case Catalog::StopMode::NotStop: 
            if (guard_.check(machine_.mPaper)) machine_.mPaper->forceStop(); 
            break;
    }

    if (machine_.mPaper->isRunning()) return false;

    guard_.reset(machine_.mPaper);
    disableOpticalTriggers();
    setPaperClutch(false);
    setThrowSwitch(false);
    machine_.swPaper->off();
    machine_.mPaper->setSpeed();
    return true;
}

}  // namespace scene
