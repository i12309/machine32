#pragma once
#include <Nextion.h>
#include "Catalog.h"

#include "../Page.h"

class pThrow: public Page {
    public:

      static pThrow& getInstance() { static pThrow instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override {
        Page::show();
        paper_switch = App::ctx().swPaper->power;
        clutch_switch = App::ctx().swCatch->power;
        setClutchUi(clutch_switch);
        setPaperUi(paper_switch);
      };

      //#####################################################################
      // Button
      NexButton bBkw    = NexButton(31, 2, "bBkw");
      NexButton bFwd    = NexButton(31, 3, "bFwd");
      NexButton bBack   = NexButton(31, 6, "bBack");
      NexButton bStop   = NexButton(31, 7, "bStop");
      NexButton bClutch = NexButton(31, 8, "bClutch");
      NexButton bPaper  = NexButton(31, 9, "bPaper");

      // Picture
      NexPicture rClutch = NexPicture(31, 10, "rClutch");
      NexPicture rPaper  = NexPicture(31, 12, "rPaper");

      // Checkbox
      NexCheckbox cCheck = NexCheckbox(31, 11, "cCheck");
      //#####################################################################

    private:
      NexTouch *nexT[7];
      bool clutch_switch = true;
      bool paper_switch = true;

      //#####################################################################

      pThrow() : Page(31, 0, "p5_Throw") {
          nexT[0] = &bBkw;
          nexT[1] = &bFwd;
          nexT[2] = &bBack;
          nexT[3] = &bStop;
          nexT[4] = &bClutch;
          nexT[5] = &bPaper;
          nexT[6] = NULL;

          bBkw.attachPop(pop_bBkw, &bBkw);
          bBkw.attachPush(push_bBkw, &bBkw);

          bFwd.attachPush(push_bFwd, &bFwd);
          bFwd.attachPop(pop_bFwd, &bFwd);

          bBack.attachPop(pop_bBack, &bBack);

          bStop.attachPop(pop_bStop, &bStop);
          bClutch.attachPop(pop_bClutch, &bClutch);
          bPaper.attachPop(pop_bPaper, &bPaper);
      }

      //#####################################################################

      static void pop_bBkw(void* ptr);
      static void push_bBkw(void* ptr);

      static void pop_bFwd(void* ptr);
      static void push_bFwd(void* ptr);

      static void pop_bBack(void* ptr);

      static void pop_bStop(void* ptr);
      static void pop_bClutch(void* ptr);
      static void pop_bPaper(void* ptr);

  // Возвращает true, когда на THROW нужно запускать сценарий PAPER+THROW.
  bool usePaperScenario() const {
      return paper_switch &&
             App::ctx().mPaper != nullptr &&
             App::ctx().swCatch != nullptr &&
             App::ctx().swPaper != nullptr;
  }

  // Запуск направления с учетом выбранных переключателей.
  void startWork(Catalog::DIR direction) {
      if (usePaperScenario()) {
          App::scene().paperWork(direction, Catalog::SPEED::Slow, true, clutch_switch);
      } else {
          App::scene().throwWork(direction, Catalog::SPEED::Slow);
      }
  }

  // Остановка того сценария, который выбран на форме.
  void stopWork(Catalog::StopMode mode) {
      // Если PAPER уже запущен, останавливаем связку PAPER+THROW независимо от текущего положения переключателя.
      if (App::scene().isPaperRunning()) App::scene().paperStop(mode);
      else App::scene().throwStop(mode);
  }

  void setClutchUi(bool enabled) {
      if (enabled) {
          bClutch.Set_background_color_bco(Catalog::Color::yellow); // Желтый
          rClutch.Set_background_image_pic(Catalog::UI::sw_pic2);
      } else {
          bClutch.Set_background_color_bco(Catalog::Color::grey); // Серый
          rClutch.Set_background_image_pic(Catalog::UI::sw_pic1);
      }
  }

  void setPaperUi(bool enabled) {
      if (enabled) {
          bPaper.Set_background_color_bco(Catalog::Color::yellow); // Желтый
          rPaper.Set_background_image_pic(Catalog::UI::sw_pic2);
      } else {
          bPaper.Set_background_color_bco(Catalog::Color::grey); // Серый
          rPaper.Set_background_image_pic(Catalog::UI::sw_pic1);
      }
  }

};
