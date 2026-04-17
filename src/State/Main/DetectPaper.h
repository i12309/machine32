#pragma once
#include "State/State.h"
#include "Core.h"

class DetectPaper : public State {
public:
    DetectPaper() : State(State::Type::DETECT_PAPER) {}

    void oneRun() override {
        State::oneRun();

        // По умолчанию поиск края идет вперед. Если в плане задан WorkParam.dir,
        // используем направление из шага.
        Catalog::DIR direction = Catalog::DIR::Forward;
        Catalog::SPEED speed = Catalog::SPEED::Custom;
        int32_t detectOffset = 0;
        Catalog::OpticalSensor optical = Catalog::OpticalSensor::EDGE;
        Catalog::ErrorCode timeoutErrorCode = Catalog::ErrorCode::PAPER_NOT_FOUND;
        const Catalog::WorkParam params = App::plan().getCurrentParams();
        if (params.hasDir) direction = params.dir;
        if (params.hasSpeed) speed = params.speed;
        if (params.hasDetectOffset) detectOffset = params.detectOffset;
        if (params.hasOptical) optical = params.optical;
        if (params.hasTimeout) timeoutErrorCode = Catalog::TimeoutCode(params.timeout, timeoutErrorCode);

        App::scene().paperDetectPaper(true, direction, speed, true, detectOffset, optical, timeoutErrorCode);
    }

    State* run() override {
        if (!App::scene().paperStop(Catalog::StopMode::NotStop)) return this;

        // Для измерения длины берём позицию, захваченную в ISR датчика,
        // а не текущую — так убираем jitter от задержки FreeRTOS.
        int32_t edgePos = App::ctx().mPaper->getCapturedPosition();
        if (App::ctx().ePaper != nullptr) edgePos = App::ctx().ePaper->getCapture();
        Log::D("DETECT_PAPER captured=%d now=%d", edgePos, App::scene().paperPosition());
        Data::param.paperPosition = edgePos;

        App::scene().tableDown(Catalog::SPEED::Normal);

        return Factory(State::Type::DONE);
    }
};
