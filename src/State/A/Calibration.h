#pragma once
#include "State/State.h"
#include "UI/Main/pWAIT.h"

class Calibration : public State {
public:
    Calibration() : State(State::Type::CALIBRATION) {}

    void oneRun() override {
        State::oneRun();

        PlanManager& plan = App::plan();
        if (plan.isActive()) return; // план уже инициализирован, не трогаем

        Data::work.task.clear();
        Data::work.task.ID=0;
        Data::work.task.MARK = 1; // всегда с меткой
        Data::work.task.OVER_mm=Data::tuning.OVER_mm; // вылет
        Data::work.task.PRODUCT_mm=0;

        Data::work.TOTAL_CYCLES = 1;
        Data::work.TOTAL_CUTS = Data::tuning.CUT_count * 2;
        if (!Data::work.valid()) return;

        Data::work.print();

        plan.beginPlan(this->type());
        if (Data::param.productCutsCount == 1) { // первый рез 
            plan.add(State::Type::TABLE_UP);
        }

        //### РАБОЧИЙ ЦИКЛ 
        plan.add(State::Type::DETECT_PAPER);
        plan.add(State::Type::DETECT_MARK);
        plan.addAction(State::Type::ACTION, &Calibration::Feed, "Feed");
        plan.add(State::Type::GUILLOTINE_FORWARD);
        plan.addAction(State::Type::ACTION, &Calibration::LoopCut, "LoopCut");
        //######

        // дальше немного отрезаем от листа что бы метка стала ближе к краю
        plan.addAction(State::Type::ACTION, &Calibration::FeedForward, "FeedForward");// сначала двигаем вперед
        plan.add(State::Type::GUILLOTINE_FORWARD);
        plan.addAction(State::Type::ACTION, &Calibration::FeedBackward, "FeedBackward");// потом отматываем назад на нужное расстояние, чтобы 10 мм до отптического датчика

        if (Data::param.productCutsCount == Data::tuning.CUT_count) { // последний рез 
            plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Step(20000));
        }

        plan.printPlan();
    }

    State* run() override {
        PlanManager& plan = App::plan();
        if (!plan.hasPending()) {
            Stats::getInstance().save();
            pWAIT::getInstance().back();
            return Factory(State::Type::SERVICE);
        }
        return Factory(plan.nextType(this->type()));
    }

private:
    static bool Feed() { Log::D(__func__);
        float mm = 0.0f; 
        // если это первый рез
        if (Data::param.cutsCount == 0) mm = -15 + (Data::tuning.MARK_LENGHT_mm + Data::tuning.OVER_mm + Data::tuning.SENSOR_DISTANCE_mm + Data::tuning.DELTA_mm);
        else mm = 15; // следующий рез 
        Log::D("Расстояние: %f",mm);
        App::ctx().mPaper->moveMM(mm, Data::work.profile.RATIO_mm,true);
        return true;
    }

    static bool LoopCut() { Log::D(__func__);
        Data::param.cutsCount++;
        Log::D("Рез: %d", Data::param.cutsCount);
        if (Data::param.cutsCount == 1) {
            PlanManager& plan = App::plan();
            
            plan.resetByActionName("Feed");
            plan.resetByType(State::Type::GUILLOTINE_FORWARD);
            plan.resetByActionName("LoopCut");
            return true;
        }
        return true;
    }

    static bool FeedForward() { Log::D(__func__);
        float mm = Data::tuning.DISTANCE_BETWEEN_MARKS_mm - Data::tuning.OVER_mm - 9.0f;
        Log::D("Расстояние: %f",mm);
        App::ctx().mPaper->moveMM(mm, Data::work.profile.RATIO_mm, true); 
        return true;
    }

    static bool FeedBackward() { Log::D(__func__);
        float mm = -1* (Data::tuning.SENSOR_DISTANCE_mm + 10.0f); 
        Log::D("Расстояние: %f",mm);
        App::ctx().mPaper->moveMM(mm, Data::work.profile.RATIO_mm, true); 
        return true;
    }
};
