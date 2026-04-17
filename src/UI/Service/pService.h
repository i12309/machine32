#pragma once
#include <Nextion.h>

#include "../Page.h"

class pService: public Page {
    public:
    
      static pService& getInstance() { static pService instance; return instance; }
    
      void loop() override { Page::loop(); nexLoop(nexT); };
    
      void show() override { Page::show();
        Data::work.clear();
        App::state()->setFactory(State::Type::SERVICE);

        if (App::ctx().swThrow == nullptr) Page::setVisible(bNext, false);
      };
    
    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[9];
    
      //##################################################################### 
    
      NexText tStatusPanel     = NexText(1, 1, "tStatusPanel");
    
      //#####################################################################

      // Button
      NexButton bTable        = NexButton(6, 8, "bTable");
      NexButton bPaper        = NexButton(6, 9, "bPaper");
      NexButton bGuillotine   = NexButton(6, 10, "bGuillotine");
      NexButton bBack         = NexButton(6, 11, "bBack");
      NexButton bSlice        = NexButton(6, 12, "bSlice");
      NexButton bCalibration  = NexButton(6, 13, "bCalibration");
      NexButton bProba        = NexButton(6, 14, "bProba");
      NexButton bNext         = NexButton(6, 15, "bNext");
    
      //#####################################################################
      pService() : Page(6, 0, "p5_Service") {
          nexT[0] = &bTable;
          nexT[1] = &bPaper;
          nexT[2] = &bGuillotine;
          nexT[3] = &bBack;
          nexT[4] = &bSlice;
          nexT[5] = &bCalibration;
          nexT[6] = &bProba;
          nexT[7] = &bNext;
          nexT[8] = NULL;

          bTable.attachPop(pop_bTable, &bTable);
          bPaper.attachPop(pop_bPaper, &bPaper);
          bGuillotine.attachPop(pop_bGuillotine, &bGuillotine);
          bBack.attachPop(pop_bBack, &bBack);
          bSlice.attachPop(pop_bSlice, &bSlice);
          bCalibration.attachPop(pop_bCalibration, &bCalibration);
          bProba.attachPop(pop_bProba, &bProba);
          bNext.attachPop(pop_bNext, &bNext);
      }
    
  //#####################################################################
  
      static void pop_bTable(void* ptr);
      static void pop_bPaper(void* ptr);
      static void pop_bGuillotine(void* ptr);
      static void pop_bBack(void* ptr);
      static void pop_bSlice(void* ptr);
      static void pop_bCalibration(void* ptr);
      static void pop_bProba(void* ptr);
      static void pop_bNext(void* ptr);
};
