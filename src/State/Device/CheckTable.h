#pragma once
#include "State/State.h"

class CheckTable : public State {
public:
    CheckTable() : State(State::Type::CHECK_TABLE) {}

    State* run() override {
        if (App::ctx().mTable == nullptr || App::ctx().sTableDown == nullptr) {
            return Factory(App::diag().addError(State::ErrorCode::TABLE, "Узел стола не найден", "", false));
        }

        App::scene().tableDown(); while (!App::scene().tableStop(Catalog::StopMode::NotStop)) { /* wait */ }
        
        delay(200);

        if (App::ctx().sTableDown->check(LOW)) {
            // еще раз 
            App::scene().tableDown(); 
            while (!App::scene().tableStop(Catalog::StopMode::NotStop)) { /* wait */ }
            // все таки нет
            if (App::ctx().sTableDown->check(LOW)) return Factory(App::diag().addFatal(State::ErrorCode::TABLE_NOT_IN));
        }
        App::ctx().mTable->setCurrentPosition(0);
        if (!Data::param.checkFull) return Factory(State::Type::DONE);

        if (!App::ctx().mTable->isRunning()) {
            App::ctx().mTable->move(5000, true);
            // если движение не удалось и датчик видит то значит нет управления мотором
            if (App::ctx().sTableDown->check(HIGH)) return Factory(App::diag().addFatal(State::ErrorCode::TABLE_NOT_OUT));
        }
        delay(200);
        if (!App::ctx().mTable->isRunning()) App::ctx().mTable->move(-5100, true);
        App::ctx().mTable->setCurrentPosition(0);
        delay(200);

        if (App::ctx().sTableDown->check(LOW)) {
            return Factory(App::diag().addFatal(State::ErrorCode::TABLE_NOT_IN));
        }

        return Factory(State::Type::DONE);
    }
};
