#pragma once
#include "State/State.h"
#include "UI/Main/pINPUT.h"
#include "UI/Profile/pProfile.h"

class Profilling : public State {
public:
    Profilling() : State(State::Type::PROFILING) {}

    void oneRun() override {
        State::oneRun();

        PlanManager& plan = App::plan();
        if (plan.isActive()) return; // план уже инициализирован, не трогаем

        plan.beginPlan(this->type());
        plan.add(State::Type::TABLE_UP);
        plan.add(State::Type::DETECT_PAPER);
        plan.addAction(State::Type::ACTION, &Profilling::FeedFirst, "FeedFirst");
        plan.add(State::Type::GUILLOTINE_FORWARD);

        // добавляем в план нужное кол-ва нам шагов
        for (int i = 0; i < Data::tuning.PROFILE_COUNT_CUT + 1; ++i) {
            plan.addAction(State::Type::ACTION, &Profilling::FeedNext, "FeedNext");
            plan.add(State::Type::GUILLOTINE_FORWARD);
        }

        plan.addAction(State::Type::ACTION, &Profilling::FeedOut, "FeedOut");
        plan.printPlan();
    }

    State* run() override {
        PlanManager& plan = App::plan();
        if (!plan.hasPending()) {
            // После завершения показать UI для ввода ширины
            ShowProfileInput();
            Stats::getInstance().save();
            return Factory(State::Type::FINISH);
        }
        return Factory(plan.nextType(this->type()));
    }

private:

    static bool FeedFirst() { Log::D(__func__);
        App::ctx().mPaper->setSpeed();
        App::ctx().mPaper->moveMM(Data::tuning.SENSOR_DISTANCE_mm + Data::tuning.DELTA_mm,Data::work.profile.RATIO_mm,true);
        return true;
    }

    static bool FeedNext() { Log::D(__func__);
        App::ctx().mPaper->move(Data::tuning.PROFILE_WIDTH_step, true);
        return true;
    }

    static bool FeedOut() { Log::D(__func__);
        App::ctx().mPaper->move(10000);
        return true;
    }

    static void ShowProfileInput() {
        // Код из GoProfile для показа UI
        pINPUT::showInput(
            "Измерение ширины",
            "Померьте ширину полосок (до сотых)",
            "Внесите получившееся значение (в мм)",
            "0",
            [](const String& widthText) {
                String trimmed = widthText;
                trimmed.trim();

                if (!T::isStringValidFloat(trimmed.c_str())) {
                    Log::D("Ввели не правильное значение");
                    return;
                }

                const float width = trimmed.toFloat();
                if (width <= 0.0f) {
                    Log::D("Ввели значение <= 0");
                    return;
                }

                Data::work.profile.RATIO_mm = (Data::tuning.PROFILE_WIDTH_step / width);

                pProfile::getInstance().show();

                pProfile& profileUI = pProfile::getInstance();
                String ratioText = String(Data::work.profile.RATIO_mm, 3);
                profileUI.tRatioMM.setText(ratioText.c_str());
                Log::D(ratioText.c_str());
            },
            []() {
                pProfile::getInstance().show();
            },
            2, // цифры
            false);
    }
};
