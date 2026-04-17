#pragma once
#include <Nextion.h>
#include "Catalog.h"

#include "../Page.h"
#include "../Main/pERROR.h"

class pTaskRun: public Page {
    public:
    
      static pTaskRun& getInstance() {static pTaskRun instance; return instance; }
    
      void loop() override { Page::loop(); nexLoop(nexT);
        switch(App::state()->type()) {
          case State::Type::IDLE: n_Idle(); break;
        } 
      };
    
      void show() override { 
        Page::show();
        if (Data::work.task.valid()) {bListTask.setText(Data::work.task.NAME.c_str());}else bListTask.setText("Выберите задание!");
        if (Data::work.profile.valid()) {bListProfile.setText(Data::work.profile.NAME.c_str());}else bListProfile.setText("Выберите профиль!");

        if (backPageStatus == Catalog::PageMode::pMain) {// показать
          tCYCLES.setText("0");
          tCYCLES.Set_font_color_pco(Catalog::Color::darkGrey);
          tLabel.Set_font_color_pco(Catalog::Color::black);

          bPlus.Set_font_color_pco(Catalog::Color::white);
          bPlus.Set_background_color_bco(Catalog::Color::yellow);

          bMinus.Set_font_color_pco(Catalog::Color::white);
          bMinus.Set_background_color_bco(Catalog::Color::yellow);
        }else {// скрыть
          tCYCLES.setText("1");
          tCYCLES.Set_font_color_pco(Catalog::Color::lightGrey);
          tLabel.Set_font_color_pco(Catalog::Color::mediumGrey);

          bPlus.Set_font_color_pco(Catalog::Color::lightGrey);
          bPlus.Set_background_color_bco(Catalog::Color::lightGrey);

          bMinus.Set_font_color_pco(Catalog::Color::lightGrey);
          bMinus.Set_background_color_bco(Catalog::Color::lightGrey);
        }

      };
    Catalog::PageMode backPageStatus; // признак куда переходить по кнопке назад 
    
    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[7];
    
      //#####################################################################
    
      // Text
      NexText tLabel        = NexText(2, 10, "tLabel");
      NexText tCYCLES       = NexText(2, 6, "tCYCLES");

      // Button
      NexButton bMinus        = NexButton(2, 2, "bMinus");
      NexButton bStart = NexButton(2, 4, "bStart");
      NexButton bBack = NexButton(2, 5, "bBack");
      NexButton bListTask  = NexButton(2, 7, "bListTask");
      NexButton bListProfile  = NexButton(2, 8, "bListProfile");
      NexButton bPlus         = NexButton(2, 9, "bPlus");
  
      //#####################################################################
      pTaskRun() : Page(2, 0, "p2_TaskRun") { 
          nexT[0] = &bMinus;
          nexT[1] = &bStart;
          nexT[2] = &bBack;
          nexT[3] = &bListTask;
          nexT[4] = &bListProfile;
          nexT[5] = &bPlus;
          nexT[6] = NULL;

          bStart.attachPop(pop_bStart, &bStart);
          bListTask.attachPop(pop_bListTask, &bListTask);
          bListProfile.attachPop(pop_bListProfile, &bListProfile);
          bBack.attachPop(pop_bBack, &bBack);
          bMinus.attachPop(pop_bMinus, &bMinus);
          bPlus.attachPop(pop_bPlus, &bPlus);
      }
    
  //#####################################################################

  static void pop_bStart(void* ptr);
  static void pop_bListTask(void* ptr);
  static void pop_bListProfile(void* ptr);
  static void pop_bBack(void* ptr);
  static void pop_bMinus(void* ptr);
  static void pop_bPlus(void* ptr);

  //#####################################################################

  /*void n_Error() override {pERROR::getInstance().show();}*/

  void n_Idle() {
    // проверяем нажата ли кнопка
    checkButton("START", pop_bStart);
  }

};
