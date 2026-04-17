#pragma once
#include "State/State.h"

// Мастер-состояние для режима SERVICE.
// Управляет повторениями через PlanManager, простые состояния должны возвращать DONE в режиме SERVICE.
class A_SERVICE : public State {
public:
    A_SERVICE() : State(State::Type::SERVICE) {}

    void oneRun() override { State::oneRun(); }

    State* run() override {return this;}

};
