#pragma once
#include "State/State.h"

class TableUp : public State {
public:
    TableUp() : State(State::Type::TABLE_UP) {}

    void oneRun() override {
        State::oneRun();
        startResult = App::scene().tableUp(Catalog::SPEED::Normal);
    }

    State* run() override {
        if (startResult == Catalog::TableActionResult::TriggerFault) {
            return Factory(App::diag().addFatal(
                State::ErrorCode::TABLE,
                "Не удалось настроить стол"
            ));
        }

        if (App::scene().isTableRunning()) return this;

        return Factory(State::Type::DONE);
    }

private:
    Catalog::TableActionResult startResult = Catalog::TableActionResult::Started;
};
