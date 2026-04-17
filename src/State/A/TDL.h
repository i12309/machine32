#pragma once

#include <cstdint>

#include "State/State.h"
#include "UI/Main/pWAIT.h"
#include "UI/Service/pSlice.h"
#include "Service/Stats.h"

class TDL : public State {
public:
    TDL() : State(State::Type::TDL) {}

    void oneRun() override {
        State::oneRun();
        PlanManager& plan = App::plan();
        if (plan.isActive()) return;
        pWAIT& wait = pWAIT::getInstance();
        plan.beginPlan(this->type());

        delta() = 1500; 
        count() = 100; 

        plan.add(State::Type::TABLE_UP);
        plan.add(State::Type::DETECT_PAPER);
        for (size_t i = 0; i < count(); i++)
        {
            plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Dir(Catalog::DIR::Forward).Step(delta()).Block(true).Speed(Catalog::SPEED::Normal));
            plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Dir(Catalog::DIR::Backward).Step(delta()).Block(true).Speed(Catalog::SPEED::Normal));
            

        }
        //plan.addAction(State::Type::ACTION, &TDL::Clear, "Clear");
        //plan.add(State::Type::DETECT_PAPER, Catalog::WorkParam().Dir(Catalog::DIR::Backward));
        //plan.addAction(State::Type::ACTION, &TDL::Report, "CCC");

        // выброс.. долго ижем край и толкаем остаток 
        //plan.add(State::Type::DETECT_MARK, Catalog::WorkParam().Timeout("PAPER_SEARCH"));
        //plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Step(5000));


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
        App::scene().tableDown(Catalog::SPEED::Normal);
        App::scene().paperMove(10000, Catalog::DIR::Forward, Catalog::SPEED::Normal);
        return true;
    }

    static int& pos() {static int value = 0;return value;}
    static int& delta() {static int value = 0;return value;}
    static int& count() {static int value = 0;return value;}

};
