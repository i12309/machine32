#pragma once
#include <Nextion.h>

#include "../Page.h"

class pKnife: public Page {
    public:

      static pKnife& getInstance() { static pKnife instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show();};

      //#####################################################################
      NexButton bKnife2  = NexButton(32, 1, "bKnife2");
      NexButton bKnife3  = NexButton(32, 2, "bKnife3");
      NexButton bKnife4  = NexButton(32, 3, "bKnife4");
      NexButton bKnife5  = NexButton(32, 4, "bKnife5");
      NexButton bKnife6  = NexButton(32, 5, "bKnife6");
      NexButton bBkw     = NexButton(32, 7, "bBkw");
      NexButton bFwd     = NexButton(32, 8, "bFwd");
      NexButton bBack    = NexButton(32, 9, "bBack");
      NexButton bStop    = NexButton(32, 10, "bStop");
      NexButton bKnife1  = NexButton(32, 11, "bKnife1");
      NexButton bParking = NexButton(32, 12, "bParking");

      // Picture
      NexCheckbox cCheck1  = NexCheckbox(32, 13, "cCheck1");
      NexCheckbox cCheck2  = NexCheckbox(32, 14, "cCheck2");
      NexCheckbox cCheck3  = NexCheckbox(32, 15, "cCheck3");
      NexCheckbox cCheck4  = NexCheckbox(32, 16, "cCheck4");
      NexCheckbox cCheck5  = NexCheckbox(32, 17, "cCheck5");
      NexCheckbox cCheck6  = NexCheckbox(32, 18, "cCheck6");
      //#####################################################################

    private:
      NexTouch *nexT[12];

      //#####################################################################

      pKnife() : Page(32, 0, "p5_Knife") {
          nexT[0] = &bKnife2;
          nexT[1] = &bKnife3;
          nexT[2] = &bKnife4;
          nexT[3] = &bKnife5;
          nexT[4] = &bKnife6;
          nexT[5] = &bBkw;
          nexT[6] = &bFwd;
          nexT[7] = &bBack;
          nexT[8] = &bStop;
          nexT[9] = &bKnife1;
          nexT[10] = &bParking;
          nexT[11] = NULL;

          bKnife2.attachPop(pop_bKnife2, &bKnife2);
          bKnife3.attachPop(pop_bKnife3, &bKnife3);
          bKnife4.attachPop(pop_bKnife4, &bKnife4);
          bKnife5.attachPop(pop_bKnife5, &bKnife5);
          bKnife6.attachPop(pop_bKnife6, &bKnife6);
          bBkw.attachPop(pop_bBkw, &bBkw);
          bFwd.attachPop(pop_bFwd, &bFwd);
          bBack.attachPop(pop_bBack, &bBack);
          bStop.attachPop(pop_bStop, &bStop);
          bKnife1.attachPop(pop_bKnife1, &bKnife1);
          bParking.attachPop(pop_bParking, &bParking);
      }

      //#####################################################################

      static void pop_bKnife1(void* ptr);
      static void pop_bKnife2(void* ptr);
      static void pop_bKnife3(void* ptr);
      static void pop_bKnife4(void* ptr);
      static void pop_bKnife5(void* ptr);
      static void pop_bKnife6(void* ptr);
      static void pop_bBkw(void* ptr);
      static void pop_bFwd(void* ptr);
      static void pop_bBack(void* ptr);
      static void pop_bStop(void* ptr);
      static void pop_bParking(void* ptr);

};
