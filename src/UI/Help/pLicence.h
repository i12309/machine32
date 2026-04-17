#pragma once
#include <Nextion.h>

#include "../Page.h"

class pLicence: public Page {
    public:
    
      static pLicence& getInstance() { static pLicence instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show(); updateLicenceText(); };
    
    private:
      NexTouch *nexT[3];
    
      //#####################################################################
    
      NexText tLicence = NexText(26, 5, "tLicence");

      // Button
      NexButton bBack    = NexButton(26, 3, "bBack");
      NexButton bUpdate  = NexButton(26, 4, "bUpdate");
      
      //#####################################################################
      pLicence() : Page(26, 0, "p4_Licence") {
        nexT[0] = &bBack;
        nexT[1] = &bUpdate;
        nexT[2] = NULL;

        bBack.attachPop(pop_bBack, &bBack);
        bUpdate.attachPop(pop_bUpdate, &bUpdate);
      }
    
  //#####################################################################

  static void pop_bBack(void* ptr);
  static void pop_bUpdate(void* ptr);

  void startLicenceRequest();
  void updateLicenceText();

};
