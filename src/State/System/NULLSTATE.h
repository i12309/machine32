#pragma once
#include "State/State.h"

class NULLSTATE : public State {
    public:
    NULLSTATE() : State(State::Type::NULL_STATE) {}

    void oneRun() override {    }

    State* run() override {return this;}
};