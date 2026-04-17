#pragma once
#include <Nextion.h>

#include "../Page.h"

class pINIT: public Page {
    public:

      static pINIT& getInstance() { static pINIT instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { 
        Page::show();    
        if (!_prefilled) prefill();  
      };

      //#####################################################################
      NexText tGroup       = NexText(34, 4, "tGroup");
      NexText tName        = NexText(34, 6, "tName");
      // Button
      NexButton bSave        = NexButton(34, 7, "bSave");
      NexButton bHTTP        = NexButton(34, 14, "bHTTP");

      // Checkbox
      NexCheckbox cAccessPoint = NexCheckbox(34, 8, "cAccessPoint");
      NexCheckbox cTest        = NexCheckbox(34, 11, "cTest");

      // Combo Box
      NexObject cbMachine    = NexObject(34, 8, "cbMachine");
      //#####################################################################

    private:
      NexTouch *nexT[3];
      bool _prefilled = false;
      uint32_t _lastMachineIndex = 0;

      //#####################################################################

      pINIT() : Page(34, 0, "p0_INIT") {
          nexT[0] = &bSave;
          nexT[1] = &bHTTP;
          nexT[2] = NULL;

          bSave.attachPop(pop_bSave, &bSave);
          bHTTP.attachPop(pop_bHTTP, &bHTTP);
      }

      //#####################################################################
      void prefill();
      void refreshMachineList();
      uint32_t getSelectedMachineIndex();
      void setSelectedMachineIndex(uint32_t index);

      static void pop_bSave(void* ptr);
      static void pop_bHTTP(void* ptr);

};
