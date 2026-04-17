#pragma once
#include <Nextion.h>
#include "Catalog.h"

#include "../Page.h"

class pTaskProcess: public Page {
    public:
    
      static pTaskProcess& getInstance() { static pTaskProcess instance; return instance; }
    
      void loop() override { Page::loop(); nexLoop(nexT);
        switch(App::state()->type()) {
          case State::Type::IDLE: n_Idle(); break;
          case State::Type::ACTION: {
            PlanManager& plan = App::plan();
            if (plan.getLastActionName() == "CutLoop") n_CutProcess(); 
            if (plan.getLastActionName() == "CycleLoop") n_CheckProcess(); 
            break;
          };
          case State::Type::FRAME: n_Frame(); break;
          case State::Type::FINISH: n_FinishCycle(); break;
        } 
      };
      void show() override { Page::show();
        if (Data::work.task.valid()) {tTask.setText(Data::work.task.NAME.c_str());} else {tTask.setText("-");}
        if (Data::work.profile.valid()) {tProfile.setText(Data::work.profile.NAME.c_str());} else {tProfile.setText("-");}

        static char info[20];
        sprintf(info, "Лист: %d", Data::param.cyclesCount);
        tCycle.setText(info);
        sprintf(info, "Рез: %d", Data::param.cutsCount);
        tCount.setText(info);

        if (Data::param.frame) {
          tPaper.setText("0");
          // показать
          Page::setVisible(tPaper, true);
          tPaperDesc.setText("Корректировка\r\n     изделия");
        } else {
          // скрыть
          Page::setVisible(tPaper, false);
          tPaperDesc.setText("");
        }
      };
    
    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[2];

      //#####################################################################

      NexText tPaper       = NexText(3, 3, "tPaper");
      NexText tPaperDesc       = NexText(3, 4, "tPaperDesc");
      NexText tCycle       = NexText(3, 6, "tCycle");
      NexText tTask       = NexText(3, 8, "tTask");
      NexText tProfile       = NexText(3, 9, "tProfile");
      NexText tCount       = NexText(3, 10, "tCount");
    
      //#####################################################################
    
      NexButton bProcess       = NexButton(3,11,"bProcess");
    
      //#####################################################################
      pTaskProcess() : Page(3, 0, "p2_TaskProcess") {
          nexT[0] = &bProcess;
          nexT[1] = NULL;  // Завершающий NULL
    
          bProcess.attachPop(pop_bProcess, &bProcess);
      }
    
  //#####################################################################

  static void pop_bProcess(void* ptr);

//#####################################################################

    void n_Idle() {
    // проверяем нажата ли кнопка 
    checkButton("START", pop_bProcess);
  }
  void n_CheckProcess() {
    if (App::mode() == State::Mode::NORMAL){ // обычный режим 
      if (_setFunc(__func__)) {
        Log::D(__func__);
        bProcess.Set_background_color_bco(63488); // Кнопка ПРЕРВАТЬ
        bProcess.setFont(3);
        bProcess.setText("СТОП");
      }
      static char info[20];
      sprintf(info, "Лист: %d", Data::param.cyclesCount);
      tCycle.setText(info);
      sprintf(info, "Рез: %d", Data::param.cutsCount);
      tCount.setText(info);
    }
  }

  void n_CutProcess() {
    //Log::L(__func__);
    if (App::mode() == State::Mode::NORMAL){ // обычный режим 
      static char info[20];
      sprintf(info, "Лист: %d", Data::param.cyclesCount);
      tCycle.setText(info);
      sprintf(info, "Рез: %d", Data::param.cutsCount);
      tCount.setText(info);
    }
  };

  void n_FinishCycle() {
    if (_setFunc(__func__))
    {
      Log::L(__func__);
      if (App::mode() == State::Mode::NORMAL) // обычный режим
      { 
        bProcess.Set_background_color_bco(Catalog::Color::green); // Кнопка вернуться
        bProcess.setFont(0);
        bProcess.setText("ВЫПОЛНЕННО!");
      }
    }
  };


  void n_Frame() {

        bProcess.Set_background_color_bco(Catalog::Color::blue); // blue button
        bProcess.setFont(3);
        bProcess.setText("Продолжить");
  }



};
