#pragma once
#include "State/State.h"
#include "Service/Stats.h"

class A_PROCESS : public State {
public:
    A_PROCESS() : State(State::Type::PROCESS) {}

    void oneRun() override {
        State::oneRun();
        Data::param.startTime = micros();

        App::mode() = State::Mode::NORMAL;

        Stats::getInstance().jobStart(Data::work.task.ID, Data::work.profile.ID, Data::work.profile.RATIO_mm);

        PlanManager& plan = App::plan();
        plan.beginPlan(this->type());

        plan.add(State::Type::TABLE_UP);
        plan.add(State::Type::DETECT_PAPER);
        if (Data::work.task.MARK) plan.add(State::Type::DETECT_MARK);

        // цикл листа 
        plan.add(State::Type::PAPER_PULL);
        plan.add(State::Type::GUILLOTINE_FORWARD);
        plan.addAction(State::Type::ACTION, &A_PROCESS::CutLoop, "CutLoop");

        // вычисляем сколько в конце надо будет сделать финишных резов что бы хвост бумаги нашинковать 
        for(int i = 0; i < Data::getFinishCUTs_count(); i++) {
            //plan.addAction(State::Type::ACTION, &A_PROCESS::FeedEnd, "FeedEnd");
            plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Mm(15).Block(true));
            plan.add(State::Type::GUILLOTINE_FORWARD);
        }

        plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Step(10000));

        // цикл процесса
        plan.addAction(State::Type::ACTION, &A_PROCESS::CycleLoop, "CycleLoop");

        plan.printPlan();
    }

    State* run() override {
        PlanManager& plan = App::plan();
        if (!plan.hasPending()) {
            Log::D("Время выполнения функции:  %f мин.",(static_cast<float>(micros() - Data::param.startTime) / 60000000.0));
            Stats::getInstance().jobFinish();
            Data::param.reset();
            App::ctx().reg.reset();
            return Factory(State::Type::FINISH);
        }

        // Это для приладки. Остановка после каждого реза продукта  
        if (Data::param.cutsCount % 2 == 1 && plan.lastType() == State::Type::GUILLOTINE_FORWARD && Data::param.frame) {
            this->setNexTypeState(plan.nextType(this->type()));
            return Factory(State::Type::FRAME);
        }
        return Factory(plan.nextType(this->type()));
    }

private:

    //static bool FeedEnd() {App::ctx().mPaper->moveMMAccum(15, Data::work.profile.RATIO_mm, true);return true;}

    static bool CutLoop() {

        // перед последним продуктовым резом листа поднять стол  
        if (Data::param.productCutsCount == Data::work.TOTAL_CUTS - 1 && Data::param.cyclesCount < Data::work.TOTAL_CYCLES){
            App::scene().tableUp(Catalog::SPEED::Normal);
        }
        
        Data::param.cutsCount++; // прибавляем рез
        if (Data::param.cutsCount >= 2 && (Data::param.cutsCount % 2 == 0)) {
            Data::param.productCutsCount++; // Продуктовая визитка формируется каждым вторым резом: 2,4,6,...
            Stats::getInstance().onProduct();
        }
        Log::D("Рез: %d, продуктовых: %d", Data::param.cutsCount, Data::param.productCutsCount);

        if (Data::param.productCutsCount < Data::work.TOTAL_CUTS) {
            PlanManager& plan = App::plan();
            plan.resetByType(State::Type::PAPER_PULL);
            plan.resetByType(State::Type::GUILLOTINE_FORWARD);
            plan.resetByActionName("CutLoop"); // сбрасываем текущий ACTION что бы в след раз еще раз запустить 
        }
        return true;
    }

    static bool CycleLoop() {
        Data::param.cyclesCount++; // считаем листы
        Stats::getInstance().onSheet();

        if (Data::param.cyclesCount < Data::work.TOTAL_CYCLES) {
            PlanManager& plan = App::plan();

            plan.resetByType(State::Type::DETECT_PAPER);
            if (Data::work.task.MARK) plan.resetByType(State::Type::DETECT_MARK);
            plan.resetByType(State::Type::PAPER_PULL);
            plan.resetByType(State::Type::GUILLOTINE_FORWARD);
            plan.resetByActionName("CutLoop");
            plan.resetByActionName("FeedEnd");
            plan.resetByType(State::Type::PAPER_MOVE);
            plan.resetByActionName("CycleLoop");// сбрасываем текущий ACTION что бы в след раз еще раз запустить
            
            Log::D("Лист: %d из %d",Data::param.cyclesCount, Data::work.TOTAL_CYCLES); 

            Data::param.cutsCount = 0;
            Data::param.productCutsCount = 0;
            Data::param.paperStepError = 0.0f;
        }
        return true;
    }
};
