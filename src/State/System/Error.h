#pragma once
#include "State/State.h"
#include "UI/Main/pERROR.h"

class Error : public State {
public:

    Error() :State(State::Type::ERROR) {};

    void oneRun() override {
        State::oneRun();
        App::plan().clear();
        pERROR::getInstance().show();

        // При ошибке останавливаем все моторы!
        App::ctx().reg.emergencyMotor();
        App::ctx().reg.emergencyClutch();
    }

    State* run() override {return this;}

    };
