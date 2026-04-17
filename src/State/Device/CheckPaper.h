#pragma once
#include "State/State.h"

class CheckPaper : public State {
public:
    CheckPaper() : State(State::Type::CHECK_PAPER) {}

    State* run() override {
        if (App::ctx().oEdge == nullptr || App::ctx().mPaper == nullptr) {
            return Factory(App::diag().addError(State::ErrorCode::PAPER, "Узел бумаги не найден", "", false));
        }

        if (App::ctx().oEdge->checkWhite()) {
            return Factory(App::diag().addFatal(State::ErrorCode::PAPER_JAM));
        }

        if (!Data::param.checkFull) {
            return Factory(State::Type::DONE);
        }

        if (App::ctx().swCatch == nullptr) {
            return Factory(App::diag().addError(State::ErrorCode::CLUTCH_PIN_NOT_CONNECT, "", "", false));
        }

        App::ctx().swCatch->on();
        delay(200);
        if (!App::ctx().mPaper->isRunning()) App::ctx().mPaper->move(1000, true);
        // TODO - тут надо прикрутить Энкодер по нему можно узнать крутиться мотор или нет
        delay(200);

        if (App::ctx().oEdge->checkWhite()) {
            return Factory(App::diag().addFatal(State::ErrorCode::PAPER_JAM));
        }

        if (!App::ctx().mPaper->isRunning()) App::ctx().mPaper->move(-1000, true);
        delay(200);
        App::ctx().swCatch->off();
        delay(200);

        if (App::ctx().oEdge->checkWhite()) {
            return Factory(App::diag().addFatal(State::ErrorCode::PAPER_JAM));
        }

        return Factory(State::Type::DONE);
    }
};
