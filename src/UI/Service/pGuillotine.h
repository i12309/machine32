#pragma once
#include <Nextion.h>
#include "Catalog.h"

#include "UI/Page.h"
#include "Controller/Registry.h"

class pGuillotine: public Page {
    public:

      static pGuillotine& getInstance() { static pGuillotine instance; return instance; }

      void loop() override { Page::loop(); nexLoop(nexT);
        switch(App::state()->type()) {
          case State::Type::SERVICE: n_Service(); break;
        }
      };

      void show() override { Page::show();
        setShowThrowUi(App::ctx().swThrow != nullptr);
        if (App::ctx().sGuillotine->check(HIGH)) { green(cCheck); } else { red(cCheck); }
        if (App::ctx().sThrow != nullptr && App::ctx().sThrow->check(HIGH)) { green(cCheckThrow); } else { red(cCheckThrow); }
      };

    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[8];

      bool throw_switch = false;

      //#####################################################################
      NexCheckbox cCheck      = NexCheckbox(12, 6, "cCheck");
      NexCheckbox cCheckThrow = NexCheckbox(12, 10, "cCheck2");
      //#####################################################################
      NexButton bBack          = NexButton(12,4,"bBack");
      NexButton bGuillotineFwd = NexButton(12,2,"bFwd");
      NexButton bGuillotineBkw = NexButton(12,3,"bBkw");
      NexButton bStopGLT       = NexButton(12,5,"bStop");

      NexButton bSlowBkw = NexButton(12,7,"bSlowBkw");
      NexButton bSlowFwd = NexButton(12,8,"bSlowFwd");

      NexButton b0     = NexButton(12,9,"b0");
      NexButton bThrow = NexButton(12,11,"bThrow");

      // Picture
      NexPicture rThrow = NexPicture(12, 12, "rThrow");
      //#####################################################################
      pGuillotine() : Page(12, 0, "p5_Guillotine") {
        nexT[0] = &bBack;
        nexT[1] = &bGuillotineFwd;
        nexT[2] = &bGuillotineBkw;
        nexT[3] = &bStopGLT;
        nexT[4] = &bSlowBkw;
        nexT[5] = &bSlowFwd;
        nexT[6] = &bThrow;
        nexT[7] = NULL;  // Завершающий NULL

        bBack.attachPop(pop_bBack, &bBack);
        bGuillotineFwd.attachPop(pop_bGuillotineFwd, &bGuillotineFwd);
        bGuillotineBkw.attachPop(pop_bGuillotineBkw, &bGuillotineBkw);
        bStopGLT.attachPop(pop_bStopGLT, &bStopGLT);

        bSlowBkw.attachPop(pop_bSlowBkw, &bSlowBkw);
        bSlowBkw.attachPush(push_bSlowBkw, &bSlowBkw);

        bSlowFwd.attachPop(pop_bSlowFwd, &bSlowFwd);
        bSlowFwd.attachPush(push_bSlowFwd, &bSlowFwd);

        bThrow.attachPop(pop_bThrow, &bThrow);
      }

  //#####################################################################

  static void pop_bBack(void* ptr);
  static void pop_bGuillotineFwd(void* ptr);
  static void pop_bGuillotineBkw(void* ptr);
  static void pop_bStopGLT(void* ptr);

  static void pop_bSlowBkw(void* ptr);
  static void push_bSlowBkw(void* ptr);

  static void pop_bSlowFwd(void* ptr);
  static void push_bSlowFwd(void* ptr);

  static void pop_bThrow(void* ptr);

  //#####################################################################
  void green(NexCheckbox& check) {
      check.Set_background_color_bco(Catalog::Color::green); // Зеленый - IN
      check.Set_font_color_pco(Catalog::Color::green);
  }

  void red(NexCheckbox& check) {
      check.Set_background_color_bco(Catalog::Color::red); // Красный - OUT
      check.Set_font_color_pco(Catalog::Color::red);
  }

  void setThrowUi() {
    if (throw_switch) {
      bThrow.Set_background_color_bco(Catalog::Color::yellow); // Желтый
      rThrow.Set_background_image_pic(Catalog::UI::sw_pic2);
    } else {
      bThrow.Set_background_color_bco(Catalog::Color::grey); // Серый
      rThrow.Set_background_image_pic(Catalog::UI::sw_pic1);
    }
  }

  void setShowThrowUi(bool hasMotor){
    if (hasMotor) {
      bThrow.setText("   Выброс");
      Page::setVisible(rThrow, true);
      throw_switch = true;
    } else {
      bThrow.setText("");
      Page::setVisible(rThrow, false);
      throw_switch = false;
    }
    setThrowUi();
  }

  void n_Service() {
      static bool sensor1 = false;
      static bool sensor2 = false;
      // Считываем каждый датчик один раз за цикл интерфейса,
      // чтобы не дублировать polling одного и того же MCP-банка.
      const bool guillotineActive = App::ctx().sGuillotine->check(HIGH);
      const bool throwActive = (App::ctx().sThrow != nullptr) ? App::ctx().sThrow->check(HIGH) : false;

      if (guillotineActive && sensor1 == false) {
        green(cCheck);
        sensor1 = true;
      }

      if (!guillotineActive && sensor1 == true) {
        red(cCheck);
        sensor1 = false;
      }

      if (App::ctx().sThrow != nullptr) {
        if (throwActive && sensor2 == false) {
          green(cCheckThrow);
          sensor2 = true;
        }

        if (!throwActive && sensor2 == true) {
          red(cCheckThrow);
          sensor2 = false;
        }
      } else if (sensor2) {
        red(cCheckThrow);
        sensor2 = false;
      }
  };

};
