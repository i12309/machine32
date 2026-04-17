// State.h
#pragma once
#include <Arduino.h>

#include "App/App.h"
#include "Core.h"
#include "Data.h"
#include "Machine/Machine.h"
#include "State/PlanManager.h"
#include "Controller/Registry.h"
#include "Controller/Trigger.h"
#include "Catalog.h"
#include "Service/DeviceError.h"
#include "State/Scene.h"

class State {

public:
    using MachineType = Catalog::MachineType;
    using Mode = Catalog::Mode;
    using Type = Catalog::State;
    using ErrorCode = Catalog::ErrorCode;
    using ErrorType = Catalog::ErrorType;

    State(Type _type): typeState(_type)
    {
        previousTypeState = Type::NULL_STATE;
        nextTypeState = Type::NULL_STATE;
    }

    virtual ~State() = default;
    static void init();
    static void process() {
        State*& current = App::state();
        if (!current) return;

        State* next = current->run();
        DeviceError& deviceErrors = App::diag();
        if (next != nullptr &&
            next->type() != State::Type::ERROR &&
            (deviceErrors.hasFatal() || (deviceErrors.hasAny() && !deviceErrors.isCollecting()))
            ){
                if (next != current) delete next;
                next = current->Factory(State::Type::ERROR);
        }

        if (next != nullptr) setState(next);
    }
    virtual void oneRun() {Log::D("         State: %s,          Mode: %s",name().c_str(),Catalog::modeName(App::mode()).c_str());};

    Type getPreviousTypeState(){ return previousTypeState; }
    void setPreviousTypeState(Type _previous){ previousTypeState = _previous;}
    Type getNextTypeState(){return nextTypeState; }
    void setNexTypeState(Type _next){nextTypeState = _next;}

    State* Factory(Type type);
    void setFactory(State::Type type){
        State*& current = App::state();
        if (current) setState(current->Factory(type));
    }

    static void setState(State* _state, bool callOneRun = true){
        State*& current = App::state();
        if(current == _state) return;
        if (current == nullptr) {
            current = _state;
            if (callOneRun && current) current->oneRun();
            return;
        }

        PlanManager& plan = App::plan();
        plan.getCurrentParams().clear();
        if (_state->type() == State::Type::DONE && plan.hasReturnTarget()) {
            Type returnState = plan.consumeReturnTarget();
            delete _state;
            setState(current->Factory(returnState), false);
            return;
        }

        _state->setPreviousTypeState(current->type());
        _state->setNexTypeState(current->getNextTypeState());
        delete current;
        current = _state;
        if (callOneRun) current->oneRun();
    }

    String name() { return Catalog::getStateName(typeState); }
    Type type(){return typeState;};

protected:
    virtual State* run() = 0;

private:
    Type typeState;
    Type previousTypeState;
    Type nextTypeState;

};
