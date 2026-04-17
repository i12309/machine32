#pragma once

#include <cstdint>

#include "State/State.h"
#include "UI/Main/pWAIT.h"
#include "UI/Service/pSlice.h"
#include "Service/Stats.h"

class T100 : public State {
public:
    T100() : State(State::Type::T100) {}

    void oneRun() override {
        State::oneRun();
        PlanManager& plan = App::plan();
        if (plan.isActive()) return;
        pWAIT& wait = pWAIT::getInstance();
        plan.beginPlan(this->type());

        delta() = 500; 
        count() = 10; 

        plan.add(State::Type::TABLE_UP);
        plan.add(State::Type::DETECT_PAPER);
        plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Dir(Catalog::DIR::Backward).Step(delta()).Block(true).Speed(Catalog::SPEED::Normal));
        plan.add(State::Type::DETECT_PAPER);
        for (size_t i = 0; i < count(); i++)
        {
            plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Dir(Catalog::DIR::Backward).Step(delta()).Block(true).Speed(Catalog::SPEED::Normal));
            plan.addAction(State::Type::ACTION, &T100::Clear, "Clear");
            plan.add(State::Type::DETECT_PAPER);
            plan.addAction(State::Type::ACTION, &T100::CCC, "CCC");
        }

        // выброс.. долго ижем край и толкаем остаток 
        plan.add(State::Type::DETECT_MARK, Catalog::WorkParam().Timeout("PAPER_SEARCH"));
        plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Step(5000));

        plan.addAction(State::Type::ACTION, &T100::Report, "Report");

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

    static bool CCC() { 
        int pos0 = delta()-App::ctx().mPaper->getCurrentPosition();
        Log::D("POS = %d", pos0);
        pos() = pos() + pos0;
        return true; 
    }

    static bool OUT() {
        //App::scene().tableDown(Catalog::SPEED::Normal);
        App::scene().paperMove(5000, Catalog::DIR::Forward, Catalog::SPEED::Normal);
        return true;
    }

    static bool Report() {
        Log::D("Средняя = %d", pos());
        return true;
    }

    static bool wait1() { delay(3000); return true; }

    static int& pos() {static int value = 0;return value;}
    static int& delta() {static int value = 0;return value;}
    static int& count() {static int value = 0;return value;}

};
