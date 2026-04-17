#pragma once
#include <Nextion.h>

#include "../Page.h"

class pBigel: public Page {
    public:

      static pBigel& getInstance() { static pBigel instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show();};

      //#####################################################################

      // Button
      NexButton bParking = NexButton(33, 2, "bParking");
      NexButton bBig     = NexButton(33, 3, "bBig");
      NexButton bBack    = NexButton(33, 4, "bBack");
      NexButton bStop    = NexButton(33, 5, "bStop");
      NexButton bDown    = NexButton(33, 7, "bDown");
      NexButton bUp      = NexButton(33, 8, "bUp");
      NexButton bDev1    = NexButton(33, 9, "bDev1");
      NexButton bDev2    = NexButton(33, 11, "bDev2");

      // Checkbox
      NexCheckbox cCheck   = NexCheckbox(33, 6, "cCheck");
      NexCheckbox cCheck2  = NexCheckbox(33, 10, "cCheck2");

      // Picture
      NexPicture rDev1    = NexPicture(33, 12, "rDev1");
      NexPicture rDev2    = NexPicture(33, 13, "rDev2");
      //#####################################################################

    private:
      NexTouch *nexT[9];

      //#####################################################################

      pBigel() : Page(33, 0, "p5_Bigel") {
          nexT[0] = &bParking;
          nexT[1] = &bBig;
          nexT[2] = &bBack;
          nexT[3] = &bStop;
          nexT[4] = &bDown;
          nexT[5] = &bUp;
          nexT[6] = &bDev1;
          nexT[7] = &bDev2;
          nexT[8] = NULL;

          bParking.attachPop(pop_bParking, &bParking);
          bBig.attachPop(pop_bBig, &bBig);
          bBack.attachPop(pop_bBack, &bBack);
          bStop.attachPop(pop_bStop, &bStop);
          bDown.attachPop(pop_bDown, &bDown);
          bUp.attachPop(pop_bUp, &bUp);
          bDev1.attachPop(pop_bDev1, &bDev1);
          bDev2.attachPop(pop_bDev2, &bDev2);
      }

      //#####################################################################

      static void pop_bParking(void* ptr);
      static void pop_bBig(void* ptr);
      static void pop_bBack(void* ptr);
      static void pop_bStop(void* ptr);
      static void pop_bDown(void* ptr);
      static void pop_bUp(void* ptr);
      static void pop_bDev1(void* ptr);
      static void pop_bDev2(void* ptr);

};
