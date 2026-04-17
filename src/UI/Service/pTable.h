#pragma once
#include <Nextion.h>
#include "Catalog.h"

#include "../Page.h"
#include "Controller/Registry.h"

class pTable: public Page {
  public:
  
    static pTable& getInstance() { static pTable instance; return instance; }
    void loop() override { Page::loop(); nexLoop(nexT); 
      switch(App::state()->type()) {
        case State::Type::SERVICE: n_Service(); break;
      }      
    };
    void show() override { Page::show();
      const bool downActive = (App::ctx().sTableDown != nullptr) ? App::ctx().sTableDown->check(HIGH) : false;
      const bool upActive = (App::ctx().sTableUp != nullptr) ? App::ctx().sTableUp->check(HIGH) : false;
      setSensorUi(cCheckDown, downActive);
      setSensorUi(cCheckUp, upActive);
    };
  
  private:
    NexTouch *nexT[7];
  
  //#####################################################################
    NexCheckbox cCheckDown = NexCheckbox(10, 6, "cCheckDown");
    NexCheckbox cCheckUp   = NexCheckbox(10, 10, "cCheckUp");

  //#####################################################################
    NexButton bBack         = NexButton(10,4,"bBack");
    NexButton bUpTable      = NexButton(10,2,"bUpTable");
    NexButton bDownTable    = NexButton(10,3,"bDownTable");
    NexButton bStopTBL      = NexButton(10,5,"bStopTBL");

    NexButton bSlowUp       = NexButton(10,7,"bSlowUp");
    NexButton bSlowDown     = NexButton(10,8,"bSlowDown");
  //#####################################################################
    pTable() : Page(10, 0, "p5_Table") {
      nexT[0] = &bBack;
      nexT[1] = &bUpTable;
      nexT[2] = &bDownTable;
      nexT[3] = &bStopTBL;
      nexT[4] = &bSlowUp;
      nexT[5] = &bSlowDown;
      nexT[6] = NULL;

      bBack.attachPop(pop_bBack, &bBack);
      bUpTable.attachPop(pop_bUpTable, &bUpTable);
      bDownTable.attachPop(pop_bDownTable, &bDownTable);
      bStopTBL.attachPop(pop_bStopTBL, &bStopTBL);

      bSlowUp.attachPop(pop_bSlowUp, &bSlowUp);
      bSlowUp.attachPush(push_bSlowUp, &bSlowUp);

      bSlowDown.attachPop(pop_bSlowDown, &bSlowDown);
      bSlowDown.attachPush(push_bSlowDown, &bSlowDown);
    }
  //#####################################################################
    static void pop_bBack(void* ptr);  
    static void pop_bUpTable(void* ptr);
    static void pop_bDownTable(void* ptr);
    static void pop_bStopTBL(void* ptr);

    static void pop_bSlowUp(void* ptr);
    static void push_bSlowUp(void* ptr);

    static void pop_bSlowDown(void* ptr);
    static void push_bSlowDown(void* ptr);

//#####################################################################
    void setSensorUi(NexCheckbox& checkbox, bool active) {
      const auto color = active ? Catalog::Color::green : Catalog::Color::red;
      checkbox.Set_background_color_bco(color);
      checkbox.Set_font_color_pco(color);
    }

    void n_Service() {
      static bool downSensor = false;
      static bool upSensor = false;
      const bool downActive = (App::ctx().sTableDown != nullptr) ? App::ctx().sTableDown->check(HIGH) : false;
      const bool upActive = (App::ctx().sTableUp != nullptr) ? App::ctx().sTableUp->check(HIGH) : false;

      if (downActive != downSensor) {
        setSensorUi(cCheckDown, downActive);
        downSensor = downActive;
      }

      if (upActive != upSensor) {
        setSensorUi(cCheckUp, upActive);
        upSensor = upActive;
      }
    };

};
