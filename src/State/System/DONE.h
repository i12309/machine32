#pragma once
#include "State/State.h"

class DONE : public State {
public:
    DONE() : State(State::Type::DONE) {}

    void oneRun() override {
        State::oneRun();
    }

    State* run() override {
        return this;
    }
};
