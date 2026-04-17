#pragma once
#include <Nextion.h>

#include "../Page.h"

class pStatistics: public Page {
    public:
    
      static pStatistics& getInstance() { static pStatistics instance; return instance; }
    
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show();};
    
    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[5];
    
      //#####################################################################
      NexButton bBackMainMenu = NexButton(7, 2, "bBackMainMenu");

      NexButton bDevice       = NexButton(7, 3, "bDevice");
      NexButton bTask         = NexButton(7, 4, "bTask");
      NexButton bProfile      = NexButton(7, 5, "bProfile");
    
      //#####################################################################
      pStatistics() : Page(7, 0, "p6_Statistics") {
          nexT[0] = &bBackMainMenu;
          nexT[1] = &bDevice;
          nexT[2] = &bTask;
          nexT[3] = &bProfile;
          nexT[4] = NULL;  // Завершающий NULL
    
          bBackMainMenu.attachPop(pop_bBackMainMenu, &bBackMainMenu);
          bDevice.attachPop(pop_bDevice, &bDevice);
          bTask.attachPop(pop_bTask, &bTask);
          bProfile.attachPop(pop_bProfile, &bProfile);
      }
    
  //#####################################################################

      static void pop_bBackMainMenu(void* ptr);
      static void pop_bDevice(void* ptr);
      static void pop_bTask(void* ptr);
      static void pop_bProfile(void* ptr);

};
