#pragma once

#include <cmath>

#include "State/State.h"
#include "UI/Main/pWAIT.h"
#include "UI/Profile/pProfile.h"

class autoPROFILE : public State {
public:
    autoPROFILE() : State(State::Type::PROFILING) {}

    void oneRun() override {
        State::oneRun();

        PlanManager& plan = App::plan();
        if (plan.isActive()) return; // План уже создан ранее, повторно не инициализируем.

        // Сбрасываем промежуточные данные автопрофилирования перед стартом новой процедуры.
        frontEdgeAbs() = -1;
        markInRel() = -1.0f;
        setFailed("", false);

        plan.beginPlan(this->type());

        // 1) Поднимаем стол и находим передний край листа.
        plan.add(State::Type::TABLE_UP);
        plan.add(State::Type::DETECT_PAPER);
        // 2) Ищем вход в метку (переход бумага -> метка).
        plan.add(State::Type::DETECT_MARK);
        plan.addAction(State::Type::ACTION, &autoPROFILE::SaveMarkIn, "SaveMarkIn");
        // 3) Ищем выход из метки (переход метка -> бумага).
        // Для этого используем уже существующее DETECT_PAPER (FALLING), без отдельного состояния.
        plan.add(State::Type::DETECT_PAPER);
        plan.addAction(State::Type::ACTION, &autoPROFILE::Calculate, "Calculate");

        plan.printPlan();
    }

    State* run() override {
        if (failedFlag()) {
            const String details = errorText();
            return Factory(App::diag().add(State::ErrorCode::PROFILING_ERROR, "Ошибка профилирования", details));
        }

        PlanManager& plan = App::plan();
        if (!plan.hasPending()) {
            // Поведение завершающего шага сохраняем близким к текущему профилированию:
            // коэффициент уже записан в Data::work.profile.RATIO_mm, возвращаем пользователя из ожидания.
            if (Page::activePage == &pWAIT::getInstance() && Page::previousPage != nullptr) pWAIT::getInstance().back();
            else pProfile::getInstance().show();

            pProfile& profileUI = pProfile::getInstance();
            String ratioText = String(Data::work.profile.RATIO_mm, 3);
            profileUI.tRatioMM.setText(ratioText.c_str());

            Stats::getInstance().save();
            return Factory(State::Type::FINISH);
        }

        return Factory(plan.nextType(this->type()));
    }

private:

    static bool SaveMarkIn() {
        frontEdgeAbs() = Data::param.paperPosition; 
        markInRel() = static_cast<float>(Data::param.markPosition - Data::param.paperPosition);
        Log::D("autoPROFILE: mark_in = %.3f steps (abs=%d)", markInRel(), Data::param.markPosition);
        return true;
    }

    static bool Calculate() {

        // относительной позиции выхода из метки (в шагах от переднего края листа).
        float markOutRel = static_cast<float>(Data::param.paperPosition - frontEdgeAbs());
        if (markOutRel <= markInRel()) {
            setFailed(String("Некорректные границы метки: mark_out=") + String(markOutRel, 3) +
                      ", mark_in=" + String(markInRel(), 3));
            return true;
        }

        // Центр метки считаем по двум фронтам, чтобы уменьшить влияние шумов оптики.
        const float markCenterSteps = (markInRel() + markOutRel) * 0.5f;
        const float markCenterDistanceMm = Data::tuning.MARK_CENTER_DISTANCE_mm;
        if (markCenterDistanceMm <= 0.0f) {
            setFailed("Параметр tuning.MARK_CENTER_DISTANCE_mm должен быть больше 0");
            return true;
        }

        const float ratioStepsPerMm = markCenterSteps / markCenterDistanceMm;
        if (ratioStepsPerMm <= 0.0f) {
            setFailed(String("Получен некорректный коэффициент: ") + String(ratioStepsPerMm, 6));
            return true;
        }

        // Валидация: измеренная ширина метки в мм должна совпадать с эталонной tuning.MARK_LENGHT_mm.
        const float measuredMarkMm = (markOutRel - markInRel()) / ratioStepsPerMm;
        const float validationDeltaMm = std::fabs(measuredMarkMm - Data::tuning.MARK_LENGHT_mm);
        if (validationDeltaMm > 0.1f) {
            setFailed(String("Провал валидации метки: измерено ") + String(measuredMarkMm, 3) +
                      " мм, ожидается " + String(Data::tuning.MARK_LENGHT_mm, 3) +
                      " мм, отклонение " + String(validationDeltaMm, 3) + " мм");
            return true;
        }

        Data::work.profile.RATIO_mm = ratioStepsPerMm;

        Log::D("autoPROFILE: mark_in=%.3f mark_out=%.3f center=%.3f", markInRel(), markOutRel, markCenterSteps);
        Log::D("autoPROFILE: ratio=%.6f steps/mm", Data::work.profile.RATIO_mm);
        Log::D("autoPROFILE: mark_mm=%.3f (delta=%.3f)", measuredMarkMm, validationDeltaMm);
        return true;
    }

    // Хранилище абсолютной позиции переднего края листа (в шагах мотора).
    static int& frontEdgeAbs() {static int value = -1; return value; }
    // Хранилище относительной позиции входа в метку (в шагах от переднего края листа).
    static float& markInRel() { static float value = -1.0f; return value; }
    // Флаг и текст ошибки профилирования.
    static bool& failedFlag() { static bool value = false; return value; }
    static String& failedText() { static String value = ""; return value; }
    static const String& errorText() {return failedText(); }
    static void setFailed(const String& text, bool failed = true) { failedFlag() = failed; failedText() = text; if (failed) Log::E("autoPROFILE: %s", text.c_str()); }
};

