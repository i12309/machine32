#pragma once

#include <initializer_list>
#include <map>

#include "Machine/Machine.h"
#include "Controller/Registry.h"
#include "Controller/McpTrigger.h"
#include "Catalog.h"
#include "Service/DeviceError.h"

// Scene - единая точка для высокоуровневых рабочих сценариев исполнительных механизмов.
// Здесь собрана общая логика запуска/остановки моторов с нужными триггерами и защитами,
// чтобы UI и состояния машины использовали одинаковое поведение без дублирования кода.
class Scene {
public:
    static Scene& getInstance() { static Scene instance; return instance; }

    // kind задаётся при restart=true и определяет уровень ошибки при срабатывании таймаута.
    // Fatal — для автоматической работы станка (блокирует повторный запуск).
    // Error — для сервисного меню (фиксирует ошибку, но позволяет перезапустить).
    bool Timeout(Catalog::ErrorCode errorCode = Catalog::ErrorCode::NO_ERROR, bool restart = false, String* details = nullptr, DeviceError::Kind kind = DeviceError::Kind::Fatal);

    // Гильотина.
    // kind — уровень ошибки при срабатывании таймаута.
    // Fatal для автоматики станка, Error для сервисного меню (позволяет перезапуск).
    bool guillotineWork(Catalog::DIR direction, uint32_t delay_ms = 0, Catalog::SPEED speed = Catalog::SPEED::Normal, DeviceError::Kind kind = DeviceError::Kind::Fatal, bool withThrow = true);
    bool guillotineStop(Catalog::StopMode mode);

    // Стол.
    Catalog::TableActionResult tableUp(Catalog::SPEED speed = Catalog::SPEED::Normal, bool blocking = false);
    Catalog::TableActionResult tableDown(Catalog::SPEED speed = Catalog::SPEED::Normal);
    // По умолчанию используем мягкую остановку, а после фактической остановки снимаем триггеры и синхронизируем позицию.
    bool tableStop(Catalog::StopMode mode = Catalog::StopMode::ForceStop);

    // Протяжка бумаги.
    // SPEED::Custom - скорость уже выставлена снаружи, внутри сценария не меняется.
    Catalog::PaperActionResult paperWork(Catalog::DIR direction, Catalog::SPEED speed = Catalog::SPEED::Normal, bool withThrow = true, bool engageClutch = false);
    Catalog::PaperActionResult paperDetectPaper(bool withThrow = true, Catalog::DIR direction = Catalog::DIR::Forward, Catalog::SPEED speed = Catalog::SPEED::Custom, bool engageClutch = true, int32_t detectOffset = 0, Catalog::OpticalSensor optical = Catalog::OpticalSensor::EDGE, Catalog::ErrorCode timeoutErrorCode = Catalog::ErrorCode::PAPER_NOT_FOUND);
    Catalog::PaperActionResult paperDetectMark(bool withThrow = true, Catalog::DIR direction = Catalog::DIR::Forward, Catalog::SPEED speed = Catalog::SPEED::Custom, bool engageClutch = false, int32_t detectOffset = 0, Catalog::OpticalSensor optical = Catalog::OpticalSensor::MARK, Catalog::ErrorCode timeoutErrorCode = Catalog::ErrorCode::PAPER_FIND_IN_MARK);
    Catalog::PaperActionResult paperMove(int32_t steps, Catalog::DIR direction, Catalog::SPEED speed = Catalog::SPEED::Normal, bool blocking = false, bool engageClutch = false, bool withThrow = true, bool correction = false);
    bool paperStop(Catalog::StopMode mode = Catalog::StopMode::ForceStop);
    // Останавливает бумагу сразу или с добегом на заданный offset после оптического триггера.
    bool paperStopOffset(int32_t offsetSteps);


    // THROW (сервис): если захват включен (App::ctx().swCatch->power), PAPER и THROW крутятся парой.
    // Если захват выключен - крутится только THROW.
    bool throwWork(Catalog::DIR direction, Catalog::SPEED speed = Catalog::SPEED::Slow);
    bool throwStop(Catalog::StopMode mode = Catalog::StopMode::SoftStop);

    // Узкие read-only фасады для State/UI, чтобы не тянуть наружу raw motor pointers.
    bool isPaperRunning() const;
    bool isGuillotineRunning() const;
    bool isTableRunning() const;
    int32_t paperPosition() const;

private:
    Scene();

    struct TimeoutContext {
        bool active = false;
        Catalog::ErrorCode errorCode = Catalog::ErrorCode::NO_ERROR;
        DeviceError::Kind kind = DeviceError::Kind::Fatal; // Уровень ошибки при срабатывании
        uint32_t startMs = 0;
        int32_t startPosition = 0;
    };
    std::map<String, TimeoutContext> timeoutCtx;

    IOptical* getOptical(Catalog::OpticalSensor sensor);
    void resetTimeout(IStepper* motor);
    bool correctPaperByEncoderTarget(int64_t encoderTarget, bool engageClutch, bool withThrow, Catalog::SPEED speed = Catalog::SPEED::Slow, int32_t tolerance = 1, uint8_t maxIterations = 5, uint32_t settleMs = 20);
    void setPaperClutch(bool engage);
    void setThrowSwitch(bool engage);
    void runMotor(IStepper* motor, Catalog::DIR direction, uint32_t delay_ms = 0, Catalog::SPEED speed = Catalog::SPEED::Normal);
    bool stopMotor(Catalog::StopMode mode, std::initializer_list<IStepper*> motors, std::initializer_list<McpTrigger::Id> triggers = {});
};
