#pragma once
#include "State/State.h"

class FINISH : public State {
    public:
    FINISH() : State(State::Type::FINISH) {}

    void oneRun() override {State::oneRun();}

    State* run() override {return this;}
};