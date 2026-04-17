#pragma once

#include <cstdint>

#include "State/State.h"
#include "UI/Main/pWAIT.h"
#include "UI/Service/pSlice.h"
#include "Service/Stats.h"

class TCUT : public State {
public:
    TCUT() : State(State::Type::TCUT) {}

    void oneRun() override {
        State::oneRun();
        PlanManager& plan = App::plan();
        if (plan.isActive()) return;
        pWAIT& wait = pWAIT::getInstance();
        plan.beginPlan(this->type());

        delta() = 3000; 
        count() = 15; 

        //plan.add(State::Type::TABLE_UP);
        plan.add(State::Type::DETECT_PAPER);
        plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Dir(Catalog::DIR::Forward).Step(3000).Block(true).Speed(Catalog::SPEED::Normal));
        plan.add(State::Type::GUILLOTINE_FORWARD);
        plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Dir(Catalog::DIR::Backward).Step(8000).Block(true).Speed(Catalog::SPEED::Normal));

        plan.printPlan();
    }

    State* run() override {
        PlanManager& plan = App::plan();
        if (!plan.hasPending()) return Factory(State::Type::SERVICE);
        return Factory(plan.nextType(this->type()));
    }

    static bool Clear() { 
        App::ctx().mPaper->setCurrentPosition(0);
        return true; 
    }

    static bool Report() { 
        Log::D("POS = %d", App::ctx().mPaper->getCurrentPosition());
        return true; 
    }

    static bool OUT() {
        //App::scene().tableDown(Catalog::SPEED::Normal);
        App::scene().paperMove(5000, Catalog::DIR::Forward, Catalog::SPEED::Normal);
        return true;
    }

    static int& pos() {static int value = 0;return value;}
    static int& delta() {static int value = 0;return value;}
    static int& count() {static int value = 0;return value;}

};
