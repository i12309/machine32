#pragma once
#include "State/State.h"
#include "Service/ESPUpdate.h"

class A_CHECK : public State {
public:
    A_CHECK() : State(State::Type::CHECK) {}

    void oneRun() override {
        State::oneRun();

        Data::param.checkFull = (getPreviousTypeState() == State::Type::BOOT);
        App::diag().clearErrors();
        App::diag().setCollecting(true);

        PlanManager& plan = App::plan();
        plan.beginPlan(this->type());
        if (! Core::settings.CHECK_SYSTEM) return;
        plan.add(State::Type::CHECK_PAPER);
        plan.add(State::Type::CHECK_GUILLOTINE);
        plan.add(State::Type::CHECK_TABLE);

        plan.printPlan();
    }

    State* run() override {
        PlanManager& plan = App::plan();
        if (!plan.hasPending()) {
            plan.clear();
            Stats::getInstance().save();
            App::diag().setCollecting(false);
            State::Type returnType = getNextTypeState();
            setNexTypeState(State::Type::NULL_STATE);
            if (returnType == State::Type::NULL_STATE) returnType = State::Type::IDLE;
            if (App::diag().hasAny()) returnType = State::Type::ERROR;
            ESPUpdate::getInstance().markCurrentFirmwareValid();
            return Factory(returnType);
        }

        return Factory(plan.nextType(this->type()));
    }
};
