#pragma once

#include <cstdint>

#include "State/State.h"
#include "UI/Main/pWAIT.h"
#include "UI/Service/pSlice.h"
#include "Service/Stats.h"

class Slice : public State {
public:
    Slice() : State(State::Type::SLICE) {}

    static void setSheetCount(int value) {
        requestedSheets() = value;
    }

    void oneRun() override {
        State::oneRun();

        PlanManager& plan = App::plan();
        if (plan.isActive()) return;

        // для сброса и мягкой подготовки с ледующему циклу - TODO подумать над тем что бы сделать отдельный метод 
        App::scene().paperStop(Catalog::StopMode::ForceStop);

        measuredCount() = 0;
        totalLengthMm() = 0.0f;
        totalLengthSteps() = 0;
        failedFlag() = false;
        failedText() = "";

        if (requestedSheets() <= 0) {
            fail("Кол-во листов должно быть больше 0");
            return;
        }
        if (Data::work.profile.RATIO_mm <= 0.0f) {
            fail("Коэф. профиля (RATIO_mm) должен быть больше 0");
            return;
        }

        updateWaitProgress(0);

        plan.beginPlan(this->type());

        //plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Step(10000));

        for (int i = 0; i < requestedSheets(); ++i) {
            plan.add(State::Type::TABLE_UP);
            addFrontEdgePlan(plan);
            plan.addAction(State::Type::ACTION, &Slice::Edge, "Edge");

            addBackEdgePlan(plan);
            plan.addAction(State::Type::ACTION, &Slice::Calculate, "Calculate");
            // выброс.. долго ижем край и толкаем остаток 
            plan.add(State::Type::DETECT_MARK, Catalog::WorkParam().Timeout("PAPER_SEARCH").Optical(Catalog::OpticalSensor::EDGE));
            plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Step(300));
        }

        // выброс.. долго ижем край и толкаем остаток 
        plan.add(State::Type::DETECT_MARK, Catalog::WorkParam().Timeout("PAPER_SEARCH").Optical(Catalog::OpticalSensor::EDGE));
        plan.add(State::Type::PAPER_MOVE,Catalog::WorkParam().Step(10000));

        plan.printPlan();

        //App::ctx().cPaper->engage();
    }

    State* run() override {
        if (failedFlag()) {
            App::scene().paperStop(Catalog::StopMode::ForceStop);
            App::scene().tableDown(Catalog::SPEED::Normal);
            return Factory(App::diag().add(State::ErrorCode::PAPER_NOT_FOUND, "Ошибка разделения листов", failedText()));
        }

        PlanManager& plan = App::plan();
        if (!plan.hasPending()) {
            App::scene().paperStop(Catalog::StopMode::NotStop);
            if (Page::activePage == &pWAIT::getInstance() && Page::previousPage != nullptr) pWAIT::getInstance().back();
            else pSlice::getInstance().show();

            setSlicePageResult();
            Stats::getInstance().save();
            return Factory(State::Type::SERVICE);
        }

        return Factory(plan.nextType(this->type()));
    }

private:

    static void addFrontEdgePlan(PlanManager& plan) {
        plan.add(State::Type::DETECT_PAPER, Catalog::WorkParam().Timeout("PAPER_SEARCH"));
    }

    static void addBackEdgePlan(PlanManager& plan) {
        plan.add(State::Type::DETECT_MARK, Catalog::WorkParam().Timeout("PAPER_SEARCH").Optical(Catalog::OpticalSensor::EDGE));
    }

    static bool Edge() {
        App::ctx().mPaper->setCurrentPosition(0);
        return true;
    }

    static bool Calculate() {
        Log::D("Data::param.markPosition = %d",Data::param.markPosition);
        if (Data::param.markPosition <= 0) {
            fail(String("Некорректная длина листа в шагах: ") + String(Data::param.markPosition));
            return true;
        }
        totalLengthMm() += static_cast<float>(Data::param.markPosition) / Data::work.profile.RATIO_mm;
        totalLengthSteps() += Data::param.markPosition;
        measuredCount()++;

        updateWaitProgress(measuredCount());

        return true;
    }

    static bool FinishMechanics() {
        App::scene().tableDown(Catalog::SPEED::Normal);
        App::scene().paperMove(6000, Catalog::DIR::Forward, Catalog::SPEED::Normal);
        return true;
    }

    static String makeSliceResultText() {
        return "ср " + String(averageLengthMm(), 2) + " мм, " + String(averageLengthSteps()) + " шагов";
    }

    static void updateWaitProgress(int currentSheet) {
        pWAIT& wait = pWAIT::getInstance();
        String statusText = "Лист " + String(currentSheet) + "/" + String(requestedSheets());
        wait.tText2.setText(statusText.c_str());
        wait.tText3.setText(makeSliceResultText().c_str());
    }

    static void setSlicePageResult() {
        pSlice::getInstance().t1.setText(makeSliceResultText().c_str());
    }

    static float averageLengthMm() {
        if (measuredCount() <= 0) return 0.0f;
        return totalLengthMm() / measuredCount();
    }

    static int averageLengthSteps() {
        if (measuredCount() <= 0) return 0;
        return static_cast<int>(totalLengthSteps() / measuredCount());
    }

    static void fail(const String& text) {
        failedFlag() = true;
        failedText() = text;
        Log::E("Slice: %s", text.c_str());
    }

    static int& requestedSheets() {
        static int value = 0;
        return value;
    }

    static int& measuredCount() {
        static int value = 0;
        return value;
    }

    static float& totalLengthMm() {
        static float value = 0.0f;
        return value;
    }

    static int64_t& totalLengthSteps() {
        static int64_t value = 0;
        return value;
    }

    static int32_t& refineBackoffSteps() {
        static int32_t value = 1;
        return value;
    }

    static bool& usePrecisionMode() {
        static bool value = false;
        return value;
    }

    static bool& failedFlag() {
        static bool value = false;
        return value;
    }

    static String& failedText() {
        static String value = "";
        return value;
    }

    static bool wait1() { delay(0); return true; }
    static bool wait2() { delay(0); return true; }
};
