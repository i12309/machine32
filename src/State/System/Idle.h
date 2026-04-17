#pragma once
#include "State/State.h"

class Idle : public State {
    public:
    Idle() : State(State::Type::IDLE) {}

    void oneRun() override {
        State::oneRun();
        Data::param.reset();
        App::ctx().reg.reset(); 
        App::mode() = Mode::NORMAL;
        // Keep warnings until user acknowledges them in UI.
        App::diag().clearErrors();
    }

    State* run() override {return this;}
};
