#pragma once
#include "State/State.h"

class DetectMark : public State {
public:
    DetectMark() : State(State::Type::DETECT_MARK) {}

    void oneRun() override {
        State::oneRun();

        // По умолчанию поиск входа в метку идет вперед. Если в плане задан WorkParam.dir,
        // используем направление из шага.
        Catalog::DIR direction = Catalog::DIR::Forward;
        Catalog::SPEED speed = Catalog::SPEED::Custom;
        int32_t detectOffset = 0;
        Catalog::OpticalSensor optical = Catalog::OpticalSensor::MARK;
        Catalog::ErrorCode timeoutErrorCode = Catalog::ErrorCode::PAPER_FIND_IN_MARK;
        const Catalog::WorkParam params = App::plan().getCurrentParams();
        if (params.hasDir) direction = params.dir;
        if (params.hasSpeed) speed = params.speed;
        if (params.hasDetectOffset) detectOffset = params.detectOffset;
        if (params.hasOptical) optical = params.optical;
        if (params.hasTimeout) timeoutErrorCode = Catalog::TimeoutCode(params.timeout, timeoutErrorCode);

        App::scene().paperDetectMark(true, direction, speed, false, detectOffset, optical, timeoutErrorCode);
    }

    State* run() override {
        if (!App::scene().paperStop(Catalog::StopMode::NotStop)) return this;

        Data::param.markPosition = App::scene().paperPosition(); // запомнили позицию
        Log::D("DETECT_MARK = %d", App::scene().paperPosition());
        return Factory(State::Type::DONE);
    }
};
