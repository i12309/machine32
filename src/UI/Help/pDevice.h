#pragma once
#include <Nextion.h>

#include "../Page.h"

class pDevice: public Page {
    public:

      static pDevice& getInstance() { static pDevice instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show();};

      //#####################################################################

      // Button
      NexButton bBack   = NexButton(15, 2, "bBack");
      NexButton bParams    = NexButton(15, 3, "bParams");
      NexButton bPinTest  = NexButton(15, 4, "bPinTest");
      NexButton bPressure      = NexButton(15, 5, "bPressure");
      NexButton b5      = NexButton(15, 6, "b5");
      NexButton b6 = NexButton(15, 7, "b6");
      NexButton bNULL      = NexButton(15, 8, "bNULL");
      //#####################################################################

    private:
      NexTouch *nexT[8];

      //#####################################################################

      pDevice() : Page(15, 0, "p4_Device") {
          nexT[0] = &bBack;
          nexT[1] = &bParams;
          nexT[2] = &bPinTest;
          nexT[3] = &bPressure;
          nexT[4] = &b5;
          nexT[5] = &b6;
          nexT[6] = &bNULL;
          nexT[7] = NULL;

          bBack.attachPop(pop_bBack, &bBack);
          bParams.attachPop(pop_bParams, &bParams);
          bPinTest.attachPop(pop_bPinTest, &bPinTest);
          bNULL.attachPop(pop_bNULL, &bNULL);
          bPressure.attachPop(pop_bPressure, &bPressure);
          b5.attachPop(pop_b5, &b5);
          b6.attachPop(pop_b6, &b6);
      }

      //#####################################################################

      static void pop_bBack(void* ptr);
      static void pop_bParams(void* ptr);
      static void pop_bPinTest(void* ptr);
      static void pop_bNULL(void* ptr);
      static void pop_bPressure(void* ptr);
      static void pop_b5(void* ptr);
      static void pop_b6(void* ptr);

};
