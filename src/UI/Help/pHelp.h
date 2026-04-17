#pragma once
#include <Nextion.h>
#include "../Page.h"

class pHelp: public Page {
    public:
      static pHelp& getInstance() { static pHelp instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show();};
    
    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[5];
    
      //#####################################################################
    
      NexButton bBack    = NexButton(8, 2, "bBack");
      NexButton bTO      = NexButton(8, 3, "bTO");
      NexButton bUpdate  = NexButton(8, 4, "bUpdate");
      NexButton bLicence = NexButton(8, 5, "bLicence");
    
      //#####################################################################
      pHelp() : Page(8, 0, "p7_Help") {
          nexT[0] = &bBack;
          nexT[1] = &bTO;
          nexT[2] = &bUpdate;
          nexT[3] = &bLicence;
          nexT[4] = NULL;
    
          bBack.attachPop(pop_bBack, &bBack);
          bTO.attachPop(pop_bTO, &bTO);
          bUpdate.attachPop(pop_bUpdate, &bUpdate);
          bLicence.attachPop(pop_bLicence, &bLicence);
      }
    
      //#####################################################################

      static void pop_bBack(void* ptr);
      static void pop_bTO(void* ptr);
      static void pop_bUpdate(void* ptr);
      static void pop_bLicence(void* ptr);

};
