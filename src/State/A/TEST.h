#pragma once

#include <cstdint>

#include "State/State.h"
#include "UI/Main/pWAIT.h"
#include "UI/Service/pSlice.h"
#include "Service/Stats.h"

class TEST : public State {
public:
    TEST() : State(State::Type::TEST) {}


    void oneRun() override {
        State::oneRun();
        PlanManager& plan = App::plan();
        if (plan.isActive()) return;
        pWAIT& wait = pWAIT::getInstance();
        plan.beginPlan(this->type());
        plan.add(State::Type::TABLE_UP);



        plan.add(State::Type::DETECT_PAPER, Catalog::WorkParam().Dir(Catalog::DIR::Forward));

        plan.addAction(State::Type::ACTION, &TEST::wait, "wait");

        plan.add(State::Type::PAPER_MOVE,
                 Catalog::WorkParam()
                     .Dir(Catalog::DIR::Backward)
                     .Step(1000)
                     .Block(true)
                     .Speed(Catalog::SPEED::Normal));


        plan.addAction(State::Type::ACTION, &TEST::wait, "wait");

        //plan.add(State::Type::DETECT_MARK, Catalog::WorkParam().Dir(Catalog::DIR::Backward));

          
        plan.add(State::Type::DETECT_PAPER,
                 Catalog::WorkParam()
                     .Dir(Catalog::DIR::Forward)
                     .Speed(Catalog::SPEED::Slow));




        plan.printPlan();
    }

    State* run() override {
        PlanManager& plan = App::plan();
        if (!plan.hasPending()) return Factory(State::Type::SERVICE);
        return Factory(plan.nextType(this->type()));
    }

        static bool wait() { delay(2000); return true; }

};
