#include "pTaskProcess.h"
#include "pTaskRun.h"

void pTaskProcess::pop_bProcess(void* ptr) {
    Log::D(__func__);
    if (App::state()->type() == State::Type::FRAME) {
        pTaskProcess& UI = pTaskProcess::getInstance();
        Data::param.delta_over = UI.getText(UI.tPaper,10).toFloat(); // получаем дельту 
        Data::work.task.PRODUCT_mm += Data::param.delta_over;
        Data::tasks.edit(Data::work.task); // сохраняем 
        UI.tPaper.setText("0"); // стираем после получения дельты что бы потом ввести новую или ничего не менять 
        pTaskProcess::getInstance().bProcess.Set_background_color_bco(63488); // Кнопка ПРЕРВАТЬ
        pTaskProcess::getInstance().bProcess.setFont(3);
        pTaskProcess::getInstance().bProcess.setText("СТОП");
        App::state()->setFactory(App::state()->getNextTypeState());
        return; 
    }

    if (App::state()->type() == State::Type::FINISH) {
        App::state()->setFactory(State::Type::CHECK); // завершаем проверкой системы, а то вдруг где что застряло 
        pTaskRun::getInstance().show();
    }
    else {
        Stats::getInstance().onReject();
        Stats::getInstance().save();
        App::diag().addWarning(State::ErrorCode::BRAKING_PROCESS, "Процесс прерван пользователем");
        //App::state()->setFactory(State::Type::CHECK);
    }
  }
