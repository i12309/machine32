#pragma once
#include "State/State.h"

class FRAME : public State {
    public:
    FRAME() : State(State::Type::FRAME) {}

    void oneRun() override {    }

    State* run() override {return this;}
};