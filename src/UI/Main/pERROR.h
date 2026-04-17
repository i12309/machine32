#pragma once
#include <Nextion.h>

#include "UI/Page.h"

class pERROR: public Page {
    public:
    
      static pERROR& getInstance() { static pERROR instance; return instance; }
    
      void loop() override { Page::loop(); nexLoop(nexT); };
    
      void show() override { Page::show();
        App::diag().resetCursor();
        renderError();
      };
    
    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[5];
    
      //#####################################################################
    
      NexText tErrorInfo1           = NexText(9, 4, "tErrorInfo1");
      NexText tErrorInfo2           = NexText(9, 5, "tErrorInfo2");
      NexText tErrorInfo3           = NexText(9, 6, "tErrorInfo3");
    
      //#####################################################################
    
      NexButton bService       = NexButton(9,3,"bService");
      NexButton bDropError       = NexButton(9,2,"bDropError");
      NexButton bNext       = NexButton(9,7,"bNext");
      NexButton bBack       = NexButton(9,8,"bBack");
    
      //#####################################################################
      pERROR() : Page(9, 0, "p01_ERROR") {
          nexT[0] = &bService;
          nexT[1] = &bDropError;
          nexT[2] = &bNext;
          nexT[3] = &bBack;
          nexT[4] = NULL;  // Завершающий NULL
    
          bService.attachPop(pop_bService, &bService);
          bDropError.attachPop(pop_bDropError, &bDropError);
          bNext.attachPop(pop_bNext, &bNext);
          bBack.attachPop(pop_bBack, &bBack);
      }
    
  //#####################################################################

  static void pop_bService(void* ptr);
  static void pop_bDropError(void* ptr);
  static void pop_bNext(void* ptr);
  static void pop_bBack(void* ptr);
  void renderError();

};



