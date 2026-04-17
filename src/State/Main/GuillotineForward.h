#pragma once
#include "State/Scene.h"

class GuillotineForward : public State {
public:
    GuillotineForward() : State(State::Type::GUILLOTINE_FORWARD) {}

    void oneRun() override {        
        State::oneRun();
        App::scene().guillotineWork(Catalog::DIR::Forward,350, Catalog::SPEED::Normal);
    }

    State* run() override {
        // Ждем когда мотор прекратит вращаться
        if (App::scene().isGuillotineRunning()) return this;
        return Factory(State::Type::DONE);
    }
};
