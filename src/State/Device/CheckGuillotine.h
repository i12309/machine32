#pragma once
#include "State/State.h"

class CheckGuillotine : public State {
public:
    CheckGuillotine() : State(State::Type::CHECK_GUILLOTINE) {}

    State* run() override {
        if (App::ctx().mGuillotine == nullptr || App::ctx().sGuillotine == nullptr) {
            return Factory(App::diag().addError(State::ErrorCode::GUILLOTINE, "Узел гильотины не найден", "", false));
        }

        int position = 0;
        uint32_t startTime = 0;

        if (Data::param.checkFull && App::ctx().sGuillotine->check(HIGH)) {
            App::ctx().mGuillotine->move(-1000, true);
            delay(200);
            // если движение не удалось и датчик видит то значит нет управления мотором
            if (App::ctx().sGuillotine->check(HIGH)) return Factory(App::diag().addFatal(State::ErrorCode::GUILLOTINE_NOT_OUT));
        }

        if (App::ctx().sGuillotine->check(LOW)) {
            position = App::ctx().mGuillotine->getCurrentPosition();
            startTime = millis();
            App::ctx().mGuillotine->runForward();
            while (1) {
                if (App::ctx().sGuillotine->check(HIGH)
                    || (abs(App::ctx().mGuillotine->getCurrentPosition() - position) >= App::ctx().mGuillotine->getCheck("GUILLOTINE_NOT_IN") )
                    || (millis() - startTime >= 5000)
                ) {
                    App::ctx().mGuillotine->forceStop();
                    break;
                }
            }
        }
        delay(200);

        if (App::ctx().sGuillotine->check(LOW)) {
            return Factory(App::diag().addFatal(State::ErrorCode::GUILLOTINE_NOT_IN));
        }

        return Factory(State::Type::DONE);
    }
};
