#pragma once

#include <cmath>

#include "State/State.h"
#include "Service/Stats.h"
#include "UI/Main/pWAIT.h"

class autoOffset : public State {
public:
    // Конструктор супер-состояния автоподбора DELTA_mm.
    // Тип оставляем CALIBRATION, чтобы сценарий встраивался в текущий сервисный поток.
    autoOffset() : State(State::Type::CALIBRATION) {}

    // Одноразовая инициализация сценария:
    // сбрасывает runtime-переменные, валидирует входные параметры и собирает план шагов.
    void oneRun() override {
        State::oneRun();

        PlanManager& plan = App::plan();
        if (plan.isActive()) return; // План уже был создан, повторная инициализация не нужна.

        iteration() = 0;
        frontEdgeAfterCutAbs() = -1;
        failedFlag() = false;
        failedText() = "";

        plan.beginPlan(this->type());

        // Подготовка листа и подъем стола выполняются один раз до цикла автоподбора.
        plan.add(State::Type::TABLE_UP);

        // Добавляем первую итерацию цикла автоподбора.
        LoopPlan();
        if (failedFlag()) return;

        plan.printPlan();
    }

    // Основной цикл супер-состояния:
    // обрабатывает ошибки, завершение плана и переход к очередному шагу.
    State* run() override {
        if (failedFlag()) {
            return Factory(App::diag().add(
                State::ErrorCode::CALIBRATION_FAILED,
                "Ошибка автоматической калибровки DELTA",
                failedText()
            ));
        }

        PlanManager& plan = App::plan();
        if (!plan.hasPending()) {
            // Сохраняем обновленную DELTA_mm только после успешного завершения цикла.
            Data::tuning.save();
            Stats::getInstance().save();

            if (Page::activePage == &pWAIT::getInstance() && Page::previousPage != nullptr) pWAIT::getInstance().back();

            return Factory(State::Type::SERVICE);
        }

        return Factory(plan.nextType(this->type()));
    }

private:
    // Добавление одной итерации цикла автоподбора в текущий план.
    // Используется и при первом запуске, и при добавлении следующей итерации.
    static void LoopPlan() {
        PlanManager& plan = App::plan();

        // Рабочий цикл подбора DELTA:
        // 1) Подводим лист в NO_PAPER (реверсом), чтобы всегда одинаково начинать поиск края.
        // 2) Ищем передний край листа и выполняем рез в расчетной линии.
        // 3) Реверсом уводим новый край за датчик.
        // 4) Снова находим новый край, ищем метку и измеряем фактическое смещение реза.
        // 5) Корректируем DELTA_mm и, при необходимости, добавляем еще одну итерацию цикла.
        plan.add(State::Type::DETECT_MARK, Catalog::WorkParam().Dir(Catalog::DIR::Backward));
        plan.add(State::Type::DETECT_PAPER);
        plan.add(State::Type::PAPER_MOVE, feedToCut());
        plan.add(State::Type::GUILLOTINE_FORWARD);
        plan.add(State::Type::DETECT_MARK, Catalog::WorkParam().Dir(Catalog::DIR::Backward));
        plan.add(State::Type::DETECT_PAPER);
        plan.addAction(State::Type::ACTION, &autoOffset::SaveAfterCut, "SaveAfterCut");
        plan.add(State::Type::DETECT_MARK);
        plan.add(State::Type::DETECT_PAPER);
        plan.addAction(State::Type::ACTION, &autoOffset::Calculate, "Calculate");
    }
    // Параметры шага PAPER_MOVE для подачи в расчетную линию реза.
    // Функция всегда рассчитывает параметры по текущему DELTA_mm в момент вызова.
    static Catalog::WorkParam feedToCut() {
        return Catalog::WorkParam()
            .Dir(Catalog::DIR::Forward)
            .setMm(Data::tuning.SENSOR_DISTANCE_mm + Data::tuning.OFFSET_FIRSTCUT_MM + Data::tuning.DELTA_mm)
            .Block(true);

        //Log::D("autoOffset: feed_to_cut travel_mm=%.4f (S2K=%.4f, D_CUT=%.4f, DELTA=%.4f)", travelMm, Data::tuning.SENSOR_DISTANCE_mm, Data::tuning.OFFSET_FIRSTCUT_MM, Data::tuning.DELTA_mm);
        //return params;
    }

    // ACTION: сохранить абсолютную позицию нового края после реза.
    static bool SaveAfterCut() {
        frontEdgeAfterCutAbs() = Data::param.paperPosition;
        return true;
    }

    // ACTION: измерить отклонение реза, скорректировать DELTA_mm и принять решение:
    // завершить сценарий, запустить следующую итерацию или перейти в ошибку.
    static bool Calculate() {
        const float ratio = Data::work.profile.RATIO_mm;
        const int markInAbs = Data::param.markPosition;
        const int markOutAbs = Data::param.paperPosition;
        // Переходим к относительным шагам от нового переднего края (после реза).
        const float markInRelSteps = static_cast<float>(markInAbs - frontEdgeAfterCutAbs());
        const float markOutRelSteps = static_cast<float>(markOutAbs - frontEdgeAfterCutAbs());
        if (markOutRelSteps <= markInRelSteps) {
            fail(String("Некорректный интервал метки: mark_in=") + String(markInRelSteps, 3) +
                 ", mark_out=" + String(markOutRelSteps, 3));
            return true;
        }

        const float markCenterSteps = (markInRelSteps + markOutRelSteps) * 0.5f;
        const float markWidthMm = (markOutRelSteps - markInRelSteps) / ratio;
        const float markWidthErrMm = std::fabs(markWidthMm - Data::tuning.MARK_LENGHT_mm);

        if (markWidthErrMm > Data::tuning.OFFSET_TOL_MM) {
            fail(String("Ширина метки вне допуска: measured=") + String(markWidthMm, 3) +
                 " мм, expected=" + String(Data::tuning.MARK_LENGHT_mm, 3) +
                 " мм, delta=" + String(markWidthErrMm, 3) + " мм");
            return true;
        }

        // Ожидаемое расстояние от нового края до центра метки после реза.
        const float expectedMm = Data::tuning.MARK_CENTER_DISTANCE_mm - Data::tuning.OFFSET_FIRSTCUT_MM;
        if (expectedMm <= 0.0f) {
            fail("Ожидаемое расстояние до центра метки <= 0, проверьте OFFSET_FIRSTCUT_MM");
            return true;
        }
        // Фактическое расстояние от нового края до центра метки.
        const float measuredMm = markCenterSteps / ratio;
        // Ошибка попадания ножа и коррекция DELTA_mm.
        const float errorMm = measuredMm - expectedMm;
        Data::tuning.DELTA_mm = Data::tuning.DELTA_mm - errorMm;

        iteration()++;
        Log::D("autoOffset: iter=%d, measured=%.4f, expected=%.4f, error=%.4f, new DELTA=%.4f",
               iteration(), measuredMm, expectedMm, errorMm, Data::tuning.DELTA_mm);

        if (std::fabs(errorMm) <= Data::tuning.OFFSET_TOL_MM) {
            Log::D("autoOffset: успех, ошибка в допуске OFFSET_TOL_MM=%.4f", Data::tuning.OFFSET_TOL_MM);
            return true;
        }

        if (iteration() >= Data::tuning.OFFSET_MAX_ITER) {
            fail(String("Не удалось выйти в допуск за ") + String(iteration()) +
                 " итераций. Остаточная ошибка=" + String(errorMm, 4) + " мм");
            return true;
        }

        // Нужна еще итерация: добавляем в план новый цикл с новым расчетом PAPER_MOVE.
        LoopPlan();
        return true;
    }


        // Счетчик итераций подбора DELTA_mm.
    static int& iteration() {
        static int value = 0;
        return value;
    }

    // Абсолютная позиция переднего края после выполнения реза.
    static int& frontEdgeAfterCutAbs() {
        static int value = -1;
        return value;
    }

    // Флаг аварийного завершения текущего сценария.
    static bool& failedFlag() {
        static bool value = false;
        return value;
    }

    // Текст детальной причины аварийного завершения.
    static String& failedText() {
        static String value = "";
        return value;
    }

    // Централизованная фиксация ошибки с логированием.
    static void fail(const String& text) {
        failedFlag() = true;
        failedText() = text;
        Log::E("autoOffset: %s", text.c_str());
    }

};
