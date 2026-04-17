#pragma once
#include "State/State.h"

class ACTION : public State {
public:
    ACTION() : State(State::Type::ACTION) {}

    void oneRun() override {
        State::oneRun();
        PlanManager::Action action = App::plan().consumeAction();
        std::string actionName = App::plan().consumeActionName();
        if (!actionName.empty()) {
            Log::D("Executing action: %s", actionName.c_str());
        }
        if (action) {
            actionSuccess = action();
        } else {
            actionSuccess = true;
        }
        if (!actionSuccess) {
            actionSuccess = (App::diag().addError(State::ErrorCode::UNKNOWN_ERROR, "Действие завершилось неуспешно", "", false) == State::Type::DONE);
        }
    }

    State* run() override {
        return Factory(actionSuccess ? State::Type::DONE : State::Type::ERROR);
    }

private:
    bool actionSuccess = true;
};
