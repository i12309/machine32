#pragma once

#include "State/State.h"

class PaperMove : public State {
public:
    PaperMove() : State(State::Type::PAPER_MOVE) {}

    // Одноразовый запуск шага перемещения бумаги по параметрам из плана.
    // Приоритет параметров:
    // 1) steps (если задан)
    // 2) mm -> конвертация в steps через RATIO_mm
    void oneRun() override {
        State::oneRun();

        const Catalog::WorkParam params = App::plan().getCurrentParams();

        direction = params.hasDir ? params.dir : Catalog::DIR::Forward;
        blocking = params.hasBlocking ? params.blocking : false;
        speed = params.hasSpeed ? params.speed : Catalog::SPEED::Normal;

        bool hasDistance = false;
        int32_t steps = 0;

        if (params.hasSteps) {
            hasDistance = true;
            steps = params.steps;
            // Если направление явно не задано, берем его из знака шага.
            if (!params.hasDir && steps < 0) direction = Catalog::DIR::Backward;
        } else if (params.hasMm) {
            hasDistance = true;
            steps = params.mm*Data::work.profile.RATIO_mm;
            // Если направление явно не задано, берем его из знака конвертации.
            if (!params.hasDir && steps < 0) direction = Catalog::DIR::Backward;
        }

        if (!hasDistance) {
            errorText = "PAPER_MOVE: не задано расстояние (steps или mm)";
            failed = true;
            return;
        }

        if (steps < 0) steps = -steps;
        requestedSteps = steps;

        if (requestedSteps == 0) {
            started = true;
            return;
        }

        App::scene().paperMove(requestedSteps, direction, speed, blocking);
        started = true;
    }

    // Ожидание завершения движения.
    // Если blocking=true, oneRun уже дождался завершения; здесь просто возвращаем DONE.
    // Если blocking=false, ждем пока мотор бумаги остановится.
    State* run() override {
        if (failed) {
            return Factory(App::diag().addError(State::ErrorCode::PAPER, errorText, "", false));
        }

        if (!started) {
            return Factory(App::diag().addError(State::ErrorCode::UNKNOWN_ERROR, "PAPER_MOVE: шаг не был запущен в oneRun", "", false));
        }
        if (requestedSteps > 0 && App::scene().isPaperRunning()) return this;

        return Factory(State::Type::DONE);
    }

private:
    Catalog::DIR direction = Catalog::DIR::Forward;
    Catalog::SPEED speed = Catalog::SPEED::Normal;
    bool blocking = false;
    bool started = false;
    bool failed = false;
    int32_t requestedSteps = 0;
    String errorText = "";
};
