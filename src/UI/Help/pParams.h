#pragma once
#include <Nextion.h>

#include "../Page.h"

class pParams: public Page {
    public:
    
      static pParams& getInstance() { static pParams instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show(); 
        cAccessPoint.setValue(Core::settings.ACCESS_POINT);

        tMachine.setText(Core::config.machine.c_str());
        tGroup.setText(Core::config.group.c_str());
        tName.setText(Core::config.name.c_str());
      };
    
    private:
      NexTouch *nexT[3];
    
      //#####################################################################
    
      NexText tMachine = NexText(28, 4, "tMachine");
      NexText tGroup = NexText(28, 6, "tGroup");
      NexText tName = NexText(28, 8, "tName");

      // Button
      NexButton bBack    = NexButton(28, 2, "bBack");
      NexButton bSave  = NexButton(28, 9, "bSave");

      
      NexCheckbox cAccessPoint = NexCheckbox(28,10,"cAccessPoint");
      
      //#####################################################################
      pParams() : Page(28, 0, "p4_Params") {
        nexT[0] = &bBack;
        nexT[1] = &bSave;
        nexT[2] = NULL;

        bBack.attachPop(pop_bBack, &bBack);
        bSave.attachPop(pop_bSave, &bSave);
      }
    
  //#####################################################################

  static void pop_bBack(void* ptr);
  static void pop_bSave(void* ptr);

};
