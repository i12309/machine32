#pragma once
#include "State/State.h"

class PaperPull : public State {
    
public:
    PaperPull() : State(State::Type::PAPER_PULL) {}

    void oneRun() override {        State::oneRun();
        App::ctx().mPaper->setSpeed(); // максимально ускоряемся для перемещении бумаги

        //Data::work.print();        Data::tuning.print();
    }

    State* run() override {
        if (App::ctx().mPaper->isRunning()) return this; // ждем завершения предыдущей операции 

        float mm = 0.0f; 

        if (Data::param.cutsCount == 0) { // если это первый рез
            // первый режим - ищем метку по которой делаем первый чистовой рез
            if (Data::work.task.MARK) mm = Data::work.task.MARK_mm + Data::work.task.OVER_mm + Data::tuning.SENSOR_DISTANCE_mm + Data::tuning.DELTA_mm; 
            // второй режим - указано расстояние от края до первого реза 
            else mm = Data::work.task.FIRST_CUT_mm + Data::tuning.EDGE_DISTANCE_mm + Data::tuning.DELTA_mm; 
        }
        else { // если это НЕ первый рез
            //  - нечётный cutsCount (только что сделали рез): подача на ширину визитки
            //  - чётный cutsCount: подача на два вылета между визитками
            if (Data::param.cutsCount % 2 == 1) {
                // если это последняя визитка прибавляем к продукту дельту последнего реза 
                if (Data::param.productCutsCount == Data::work.TOTAL_CUTS - 1) mm = Data::work.task.PRODUCT_mm + Data::work.task.LASTCUT_mm;
                else mm = Data::work.task.PRODUCT_mm;// чистовой по изделию
            }
            else mm = (Data::work.task.OVER_mm * 2)+Data::param.delta_over;// разделительный (два вылета)
        } 

        Log::D("Расстояние: %f",mm);
        App::ctx().mPaper->moveMMAccum(mm, Data::work.profile.RATIO_mm, true);
        return Factory(State::Type::DONE); 
    }

};
