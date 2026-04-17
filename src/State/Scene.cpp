#include "Scene.h"

#include <cstdlib>
#include <cmath>
#include <limits>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "App/App.h"
#include "Controller/McpTrigger.h"
#include "Controller/Trigger.h"
#include "Service/DeviceError.h"
#include "Service/Log.h"

Scene::Scene() {}

void Scene::setPaperClutch(bool engage) {
    // Важный момент для общей STEP-линии: переключаем ENABLE только когда
    // генератор шагов точно "тихий", иначе можно поймать рывок на хвосте очереди.
    if (!App::ctx().mPaper->isRunning()) vTaskDelay(pdMS_TO_TICKS(2));
    if (engage) App::ctx().swCatch->on(); else App::ctx().swCatch->off();
}

void Scene::setThrowSwitch(bool engage) {
    if (!App::ctx().mPaper->isRunning()) vTaskDelay(pdMS_TO_TICKS(2));
    if (engage) App::ctx().swThrow->on(); else App::ctx().swThrow->off();
}

void Scene::resetTimeout(IStepper* motor) {
    if (motor == nullptr) return;
    const std::string& motorNameStd = motor->getMotorName();
    if (motorNameStd.empty()) return;
    const String motorName = String(motorNameStd.c_str());

    TimeoutContext& timeoutState = this->timeoutCtx[motorName];
    timeoutState.active = false;
    timeoutState.errorCode = Catalog::ErrorCode::NO_ERROR;
    timeoutState.startMs = 0;
    timeoutState.startPosition = 0;
}

bool Scene::correctPaperByEncoderTarget(int64_t encoderTarget, bool engageClutch, bool withThrow, Catalog::SPEED speed, int32_t tolerance, uint8_t maxIterations, uint32_t settleMs) {
    if (App::ctx().mPaper == nullptr || App::ctx().ePaper == nullptr) return false;

    int64_t prevAbsError = std::numeric_limits<int64_t>::max();

    for (uint8_t attempt = 0; attempt < maxIterations; ++attempt) {
        if (settleMs > 0) vTaskDelay(pdMS_TO_TICKS(settleMs));

        const int64_t encoderNow = App::ctx().ePaper->getCount();
        const int32_t motorPosition = App::ctx().mPaper->getCurrentPosition();
        const int64_t error = encoderTarget - encoderNow;
        const int64_t absError = llabs(error);

        Log::D("ENC trim[%u]: target=%lld, encoder=%lld, motor=%ld, error=%lld",
               attempt,
               static_cast<long long>(encoderTarget),
               static_cast<long long>(encoderNow),
               static_cast<long>(motorPosition),
               static_cast<long long>(error));

        if (absError <= tolerance) return true;

        if (absError >= prevAbsError) {
            Log::D("ENC trim stop: no progress (prev=%lld, now=%lld)",
                   static_cast<long long>(prevAbsError),
                   static_cast<long long>(absError));
            return false;
        }

        if (error < std::numeric_limits<int32_t>::min() || error > std::numeric_limits<int32_t>::max()) {
            Log::E("ENC trim overflow: error=%lld", static_cast<long long>(error));
            return false;
        }

        prevAbsError = absError;

        setPaperClutch(engageClutch);
        setThrowSwitch(withThrow);
        App::ctx().swPaper->on();
        App::ctx().mPaper->setSpeed(speed);
        App::ctx().mPaper->move(static_cast<int32_t>(error), true);

        //while (!paperStop(Catalog::StopMode::NotStop)) vTaskDelay(pdMS_TO_TICKS(5));
    }

    const int64_t finalError = encoderTarget - App::ctx().ePaper->getCount();
    Log::D("ENC trim limit reached: error=%lld", static_cast<long long>(finalError));
    return llabs(finalError) <= tolerance;
}

bool Scene::Timeout(Catalog::ErrorCode errorCode, bool restart, String* details, DeviceError::Kind kind) {
    if (restart) {
        const String timeoutName = Catalog::TimeoutName(errorCode);
        if (errorCode == Catalog::ErrorCode::NO_ERROR || timeoutName.length() == 0) return false;
        if (details != nullptr) *details = "";
        for (const auto& pair : App::ctx().reg.getMotors()) {
            IStepper* motor = pair.second;
            if (motor == nullptr || !motor->isRunning()) continue;

            TimeoutContext& timeoutState = this->timeoutCtx[pair.first];
            timeoutState.active = true;
            timeoutState.errorCode = errorCode;
            timeoutState.kind = kind; // Сохраняем уровень ошибки для момента срабатывания
            timeoutState.startMs = millis();
            timeoutState.startPosition = motor->getCurrentPosition();
        }
        return false;
    }

    if (details != nullptr) *details = "";
    bool timeoutHit = false;
    String combinedDetails = "";

    for (const auto& pair : App::ctx().reg.getMotors()) {
        IStepper* motor = pair.second;
        if (motor == nullptr) continue;

        TimeoutContext& timeoutState = this->timeoutCtx[pair.first];
        if (!timeoutState.active) continue;
        if (errorCode != Catalog::ErrorCode::NO_ERROR && timeoutState.errorCode != errorCode) continue;

        if (!motor->isRunning()) {
            resetTimeout(motor);
            continue;
        }

        const String timeoutName = Catalog::TimeoutName(timeoutState.errorCode);
        if (timeoutName.length() == 0) {
            resetTimeout(motor);
            continue;
        }

        const int32_t maxSteps = motor->getCheck(timeoutName.c_str(), -1);
        const int32_t maxMs = motor->getCheckMs(timeoutName.c_str(), -1);
        if (maxSteps <= 0 && maxMs <= 0) continue;

        const uint32_t elapsedMs = millis() - timeoutState.startMs;
        const int32_t elapsedSteps = std::abs(motor->getCurrentPosition() - timeoutState.startPosition);

        const bool stepTimeout = (maxSteps > 0 && elapsedSteps >= maxSteps);
        const bool timeTimeout = (maxMs > 0 && elapsedMs >= static_cast<uint32_t>(maxMs));
        if (!stepTimeout && !timeTimeout) continue;

        String hitDetails = "Motor=" + pair.first +
                            ", mode=" + timeoutName +
                            ", steps=" + String(elapsedSteps) + "/" + String(maxSteps) +
                            ", ms=" + String(elapsedMs) + "/" + String(maxMs);
        String shortDetails = "Превышен timeout для "+pair.first;
        // Уровень ошибки задан при старте таймаута: Fatal для автоматики, Error для сервиса.
        switch (timeoutState.kind) {
            case DeviceError::Kind::Fatal:   App::diag().addFatal(timeoutState.errorCode, "", shortDetails); break;
            case DeviceError::Kind::Error:   App::diag().addError(timeoutState.errorCode, "", shortDetails); break;
            case DeviceError::Kind::Warning: App::diag().addWarning(timeoutState.errorCode, shortDetails);   break;
        }
        if (details != nullptr) {
            if (combinedDetails.length() > 0) combinedDetails += "\n";
            combinedDetails += hitDetails;
        }
        timeoutHit = true;
        resetTimeout(motor);
    }

    if (details != nullptr) *details = combinedDetails;
    return timeoutHit;
}

//###############################################################################################

IOptical* Scene::getOptical(Catalog::OpticalSensor sensor) {
    switch (sensor) {
        case Catalog::OpticalSensor::MARK:
            return App::ctx().oMark;
        case Catalog::OpticalSensor::EDGE:
        default:
            return App::ctx().oEdge;
    }
}

void Scene::runMotor(IStepper* motor, Catalog::DIR direction, uint32_t delay_ms, Catalog::SPEED speed) {
    if (motor == nullptr) return;
    if (speed != Catalog::SPEED::Custom) motor->setSpeed(speed);

    switch (direction) {
        case Catalog::DIR::Forward: motor->runForward(); break;
        case Catalog::DIR::Backward: motor->runBackward(); break;
    };

    if (delay_ms > 0) vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

bool Scene::stopMotor(Catalog::StopMode mode, std::initializer_list<IStepper*> motors, std::initializer_list<McpTrigger::Id> triggers) {
    for (IStepper* motor : motors) {
        if (motor == nullptr) continue;
        switch (mode) {
            case Catalog::StopMode::ForceStop: motor->forceStop(); break;
            case Catalog::StopMode::SoftStop: motor->stopMove(); break;
            case Catalog::StopMode::NotStop: break;
        }
    }

    for (IStepper* motor : motors) {
        if (motor != nullptr && motor->isRunning()) return false;
    }

    for (McpTrigger::Id trigger : triggers) {
        App::ctx().mcpTrigger.disarm(trigger);
    }

    for (IStepper* motor : motors) {
        if (motor == nullptr) continue;
        motor->setSpeed();
        resetTimeout(motor);
    }

    return true;
}

//###############################################################################################
// ГИЛЬОТИНА

bool Scene::guillotineWork(Catalog::DIR direction, uint32_t delay_ms, Catalog::SPEED speed, DeviceError::Kind kind, bool withThrow) {
    if (withThrow && App::ctx().swThrow != nullptr) App::ctx().mcpTrigger.arm(McpTrigger::Id::THROW_FORCE);
    //else App::ctx().mcpTrigger.disarm(McpTrigger::Id::THROW_FORCE);
    runMotor(App::ctx().mGuillotine, direction, delay_ms, speed);
    App::ctx().mcpTrigger.arm(McpTrigger::Id::GUILLOTINE_HOME);

    // Запускаем таймаут и активируем watchdog после фактического старта мотора.
    // GUILLOTINE_TIMEOUT (Trigger) будет вызывать guillotineStop(NotStop) каждые 10 мс пока активен.
    resetTimeout(App::ctx().mGuillotine);
    Timeout(Catalog::ErrorCode::GUILLOTINE_NOT_IN, true, nullptr, kind);
    Trigger::getInstance().activateTrigger("GUILLOTINE_TIMEOUT");

    return true;
}

bool Scene::guillotineStop(Catalog::StopMode mode) {
    if (mode == Catalog::StopMode::NotStop) {
        // Проверяем активные таймауты без подачи команды остановки.
        // Если таймаут сработал — принудительно останавливаем мотор и пишем ошибку.
        String details;
        if (Timeout(Catalog::ErrorCode::NO_ERROR, false, &details)) {
            Log::D("Guillotine timeout: %s", details.c_str());
            App::ctx().mGuillotine->forceStop();
        }
    }

    if (!stopMotor(mode, {App::ctx().mGuillotine}, {McpTrigger::Id::GUILLOTINE_HOME, McpTrigger::Id::THROW_FORCE})) return false;

    // Мотор остановлен — деактивируем watchdog.
    Trigger::getInstance().deactivateTrigger("GUILLOTINE_TIMEOUT");
    return true;
}

//###############################################################################################
// СТОЛ

Catalog::TableActionResult Scene::tableUp(Catalog::SPEED speed, bool blocking) {
    Log::D(__func__);
    // После предыдущего движения вниз триггер TABLE_HOME мог остаться взведён
    // до полного завершения остановки. Снимаем его до проверки лимита вверх,
    // чтобы читать реальный уровень датчика, а не кэш при blocked polling.
    App::ctx().mcpTrigger.disarm(McpTrigger::Id::TABLE_HOME);

    if (App::ctx().sTableUp->check(HIGH)) {
        return Catalog::TableActionResult::AtLimit;
    }

    // До старта подъема обязательно взводим верхний лимит:
    // если защита не поднялась, движение запускать нельзя.
    if (!App::ctx().mcpTrigger.arm(McpTrigger::Id::TABLE_UP_LIMIT)) {
        Log::L("[Scene] %s failed: TABLE_UP_LIMIT trigger was not armed", __func__);
        return Catalog::TableActionResult::TriggerFault;
    }
    runMotor(App::ctx().mTable, Catalog::DIR::Forward, 0, speed);

    // Запускаем таймаут и активируем мягкий watchdog после фактического старта мотора.
    // TABLE_TIMEOUT (Trigger) будет вызывать tableStop(NotStop) каждые 10 мс пока активен.
    resetTimeout(App::ctx().mTable);
    Timeout(Catalog::ErrorCode::TABLE_NOT_UP, true);
    Trigger::getInstance().activateTrigger("TABLE_TIMEOUT");

    if (blocking) {
        // Используем tableStop(NotStop) вместо простой проверки isRunning(),
        // чтобы таймаут отрабатывал и в блокирующем режиме.
        while (!tableStop(Catalog::StopMode::NotStop)) vTaskDelay(pdMS_TO_TICKS(5));
    }

    return Catalog::TableActionResult::Started;
}

Catalog::TableActionResult Scene::tableDown(Catalog::SPEED speed) {
    Log::D(__func__);
    // Снимаем оба "столовых" триггера до pre-check:
    // если после ручной остановки один из них остался armed, ISensor::check()
    // будет читать кэш из-за blocked polling и даст ложный AtLimit.
    App::ctx().mcpTrigger.disarm(McpTrigger::Id::TABLE_UP_LIMIT);
    App::ctx().mcpTrigger.disarm(McpTrigger::Id::TABLE_HOME);

    if (App::ctx().sTableDown->check(HIGH)) {
        // Если уже в "доме", жестко нормализуем позицию.
        App::ctx().mTable->setCurrentPosition(0); // TODO - это кажется уже лишнее и можно везде удалить 
        return Catalog::TableActionResult::AtLimit;
    }

    // Перед движением вниз нужен "домашний" триггер,
    // иначе стол не сможет корректно остановиться и синхронизировать ноль.
    if (!App::ctx().mcpTrigger.arm(McpTrigger::Id::TABLE_HOME)) {
        Log::L("[Scene] %s failed: TABLE_HOME trigger was not armed", __func__);
        return Catalog::TableActionResult::TriggerFault;
    }
    runMotor(App::ctx().mTable, Catalog::DIR::Backward, 0, speed);

    // Fallback: если датчик уже пойман, останавливаем и синхронизируем позицию сразу.
    if (App::ctx().sTableDown->check(HIGH)) {
        App::ctx().mTable->forceStop();
        App::ctx().mTable->setCurrentPosition(0);
        App::ctx().mcpTrigger.disarm(McpTrigger::Id::TABLE_HOME);
        return Catalog::TableActionResult::AtLimit;
    }

    // Запускаем таймаут и активируем мягкий watchdog: мотор реально запущен и едет к датчику.
    // TABLE_TIMEOUT (Trigger) будет вызывать tableStop(NotStop) каждые 10 мс пока активен.
    resetTimeout(App::ctx().mTable);
    Timeout(Catalog::ErrorCode::TABLE_NOT_DOWN, true);
    Trigger::getInstance().activateTrigger("TABLE_TIMEOUT");

    return Catalog::TableActionResult::Started;
}

bool Scene::tableStop(Catalog::StopMode mode) {
    if (mode == Catalog::StopMode::NotStop) {
        // Проверяем активные таймауты без подачи команды остановки.
        // Если таймаут сработал — принудительно останавливаем мотор стола и пишем ошибку.
        String details;
        if (Timeout(Catalog::ErrorCode::NO_ERROR, false, &details)) {
            Log::D("Table timeout: %s", details.c_str());
            App::ctx().mTable->forceStop();
        }
    }

    if (!stopMotor(mode, {App::ctx().mTable}, {McpTrigger::Id::TABLE_HOME, McpTrigger::Id::TABLE_UP_LIMIT})) return false;

    // Если в момент остановки датчик дома активен - фиксируем нулевую позицию.
    if (App::ctx().sTableDown->check(HIGH)) App::ctx().mTable->setCurrentPosition(0);

    // Мотор остановлен — деактивируем watchdog, чтобы он не крутился вхолостую.
    Trigger::getInstance().deactivateTrigger("TABLE_TIMEOUT");
    return true;
}

//###############################################################################################
// ПРОТЯЖКА БУМАГИ

Catalog::PaperActionResult Scene::paperWork(Catalog::DIR direction, Catalog::SPEED speed, bool withThrow, bool engageClutch) {
    Log::D(__func__);
    setPaperClutch(engageClutch);
    setThrowSwitch(withThrow);
    App::ctx().swPaper->on();
    runMotor(App::ctx().mPaper, direction, 0, speed);

    return withThrow ? Catalog::PaperActionResult::Started : Catalog::PaperActionResult::StartedWithoutThrow;
}

Catalog::PaperActionResult Scene::paperDetectPaper(bool withThrow, Catalog::DIR direction, Catalog::SPEED speed, bool engageClutch, int32_t detectOffset, Catalog::OpticalSensor opticalSensor, Catalog::ErrorCode timeoutErrorCode) {
    Log::D(__func__);
    IOptical* optical = getOptical(opticalSensor);
    // Уже на бумаге (white): поиск фронта не запускаем.
    if (optical->checkWhite()) {
        resetTimeout(App::ctx().mPaper);
        return Catalog::PaperActionResult::Finished;
    }

    if (speed == Catalog::SPEED::Custom) {
        App::ctx().mPaper->setSpeed("Paper");
    } else {
        App::ctx().mPaper->setSpeed(speed);
    }

    Catalog::PaperActionResult result;
    if (detectOffset == 0) result = paperWork(direction, speed, withThrow, engageClutch);
    else result = paperMove(1000000, direction, speed, false, engageClutch, withThrow, false);

    resetTimeout(App::ctx().mPaper);
    Timeout(timeoutErrorCode, true);

    const int32_t signedOffset = (direction == Catalog::DIR::Backward) ? -static_cast<int32_t>(abs(detectOffset)) : static_cast<int32_t>(abs(detectOffset));
    optical->enableTrigger(FALLING, signedOffset);

    return result;
}

Catalog::PaperActionResult Scene::paperDetectMark(bool withThrow, Catalog::DIR direction, Catalog::SPEED speed, bool engageClutch, int32_t detectOffset, Catalog::OpticalSensor opticalSensor, Catalog::ErrorCode timeoutErrorCode) {
    Log::D(__func__);
    IOptical* optical = getOptical(opticalSensor);
    if (optical->checkBlack()) {
        resetTimeout(App::ctx().mPaper);
        return Catalog::PaperActionResult::Finished;
    }

    if (speed == Catalog::SPEED::Custom) {
        App::ctx().mPaper->setSpeed("Mark");
    } else {
        App::ctx().mPaper->setSpeed(speed);
    }

    Catalog::PaperActionResult result;
    if (detectOffset == 0) result = paperWork(direction, speed, withThrow, engageClutch);
    else result = paperMove(1000000, direction, speed, false, engageClutch, withThrow, false);

    resetTimeout(App::ctx().mPaper);
    Timeout(timeoutErrorCode, true);

    const int32_t signedOffset = (direction == Catalog::DIR::Backward) ? -static_cast<int32_t>(abs(detectOffset)) : static_cast<int32_t>(abs(detectOffset));
    optical->enableTrigger(RISING, signedOffset);

    return result;
}

Catalog::PaperActionResult Scene::paperMove(int32_t steps, Catalog::DIR direction, Catalog::SPEED speed, bool blocking, bool engageClutch, bool withThrow, bool correction){
    Log::D(__func__);
    setPaperClutch(engageClutch);
    setThrowSwitch(withThrow);
    App::ctx().swPaper->on();
    if (speed != Catalog::SPEED::Custom) {
        App::ctx().mPaper->setSpeed(speed);
    }

    int32_t signedSteps = (direction == Catalog::DIR::Backward) ? -steps : steps;
    const int64_t encoderStart = (blocking && correction) ? App::ctx().ePaper->getCount() : 0;
    const int64_t encoderTarget = encoderStart + static_cast<int64_t>(signedSteps);
    App::ctx().mPaper->move(signedSteps, false);

    if (blocking) {
        while (!paperStop(Catalog::StopMode::NotStop)) vTaskDelay(pdMS_TO_TICKS(5));
        if (correction) {
            correctPaperByEncoderTarget(encoderTarget, engageClutch, withThrow);
            return Catalog::PaperActionResult::Finished;
        }
        /*if (correction){// для корректировки 
            const int64_t enc = App::ctx().ePaper->getCount();
            const int32_t pos = App::ctx().mPaper->getCurrentPosition();
            const int64_t delta = pos - enc;
            Log::D("ПОКАЗАНИЯ! Шаги:%ld, Энкодер:%lld, Мотор:%ld, разница:%lld",
                (long)steps, (long long)enc, (long)pos, (long long)delta);

            if (abs(delta) > 0) {
                delay(1000);
                App::ctx().mPaper->move(delta, true);
                Log::D("Коррекция! Шаги:%ld, Энкодер:%lld, Мотор:%ld, разница:%lld",
                (long)delta, (long long)App::ctx().ePaper->getCount(), (long)App::ctx().mPaper->getCurrentPosition(), (long long)(App::ctx().mPaper->getCurrentPosition() - App::ctx().ePaper->getCount()) );
            }
            //App::ctx().ePaper->pauseCount();
        }*/
        return Catalog::PaperActionResult::Finished;
    }

    return withThrow ? Catalog::PaperActionResult::Started : Catalog::PaperActionResult::StartedWithoutThrow;
}

// Используется в IOptical::taskTrigger:
// после оптического срабатывания нужно либо остановиться сразу, либо аккуратно добежать на offset.
bool Scene::paperStopOffset(int32_t offsetSteps) {
    if (offsetSteps == 0) {
        return paperStop(Catalog::StopMode::ForceStop);
    }

    App::ctx().oMark->disableTrigger();
    App::ctx().oEdge->disableTrigger();

    const int32_t target = App::ctx().mPaper->getCurrentPosition() + offsetSteps;

    const int64_t offsetAbs = llabs(static_cast<long long>(offsetSteps));
    const int64_t paperStopPath = llabs(static_cast<long long>(App::ctx().mPaper->stepsToStop()));
    const int64_t maxStopPath = paperStopPath;

    if (maxStopPath > offsetAbs) {
        // Текущего тормозного пути больше, чем требуемый offset:
        // останавливаемся жестко и доезжаем корректирующим move до целевой точки.
        App::ctx().mPaper->forceStop();

        const int32_t paperCorrection = target - App::ctx().mPaper->getCurrentPosition();
        if (paperCorrection != 0) App::ctx().mPaper->move(paperCorrection, false);

    } else {
        // Запас по тормозному пути есть, значит можно сразу перевести оба мотора в moveTo(target).
        App::ctx().mPaper->moveTo(target, false);
    }

    while (!paperStop(Catalog::StopMode::NotStop)) vTaskDelay(pdMS_TO_TICKS(5));

    return true;
}

bool Scene::paperStop(Catalog::StopMode mode) {
    switch (mode) {
        case Catalog::StopMode::ForceStop:
            App::ctx().mPaper->forceStop();
            
            resetTimeout(App::ctx().mPaper);
            App::ctx().oMark->disableTrigger();
            App::ctx().oEdge->disableTrigger();
            break;

        case Catalog::StopMode::SoftStop:
            App::ctx().mPaper->stopMove();

            resetTimeout(App::ctx().mPaper);
            App::ctx().oMark->disableTrigger();
            App::ctx().oEdge->disableTrigger();
            break;

        case Catalog::StopMode::NotStop:
            {
                String details;
                if (Timeout(Catalog::ErrorCode::NO_ERROR, false, &details)) {
                    Log::D("Paper timeout: %s", details.c_str());
                    App::ctx().mPaper->forceStop();
                }
            }
            break;
    }

    if (App::ctx().mPaper->isRunning()) return false;

    resetTimeout(App::ctx().mPaper);
    App::ctx().oMark->disableTrigger();
    App::ctx().oEdge->disableTrigger();
    setPaperClutch(false);
    setThrowSwitch(false);
    App::ctx().swPaper->off();
    App::ctx().mPaper->setSpeed();
    return true;
}

//###############################################################################################
// ВЫБРОС

bool Scene::throwWork(Catalog::DIR direction, Catalog::SPEED speed) {
    setThrowSwitch(true);
    runMotor(App::ctx().mPaper, direction, 0, speed);
    return true;
}

bool Scene::throwStop(Catalog::StopMode mode) {
    if (!stopMotor(mode, {App::ctx().mPaper})) return false;
    setThrowSwitch(false);
    return true;
}

bool Scene::isPaperRunning() const {
    return App::ctx().mPaper != nullptr && App::ctx().mPaper->isRunning();
}

bool Scene::isGuillotineRunning() const {
    return App::ctx().mGuillotine != nullptr && App::ctx().mGuillotine->isRunning();
}

bool Scene::isTableRunning() const {
    return App::ctx().mTable != nullptr && App::ctx().mTable->isRunning();
}

int32_t Scene::paperPosition() const {
    if (App::ctx().mPaper == nullptr) return 0;
    return App::ctx().mPaper->getCurrentPosition();
}
