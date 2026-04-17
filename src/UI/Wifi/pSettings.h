#pragma once
#include <Nextion.h>

#include "../Page.h"

class pSettings: public Page {
    public:
    
      static pSettings& getInstance() { static pSettings instance; return instance; }
    
      void loop() override { Page::loop(); nexLoop(nexT); };
    
      void show() override { Page::show();

      };
    
    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[4];
    
      //#####################################################################
    
      NexButton bBackMainMenu     = NexButton(5,4,"bBackMainMenu");
      NexButton bWifi             = NexButton(5,2,"bWifi");
      NexButton bDevice           = NexButton(5,6,"bDevice");
      
          
      //#####################################################################
      pSettings() : Page(5, 0, "p4_Settings") {
        nexT[0] = &bBackMainMenu;
        nexT[1] = &bWifi;
        nexT[2] = &bDevice;
        nexT[3] = NULL;  // Завершающий NULL
  
        bBackMainMenu.attachPop(pop_bBackMainMenu, &bBackMainMenu);
        bWifi.attachPop(pop_bWifi, &bWifi);
        bDevice.attachPop(pop_bDevice, &bDevice);
      }
    
  //#####################################################################

  static void pop_bBackMainMenu(void* ptr);
  static void pop_bWifi(void* ptr);
  static void pop_bDevice(void* ptr);

};
