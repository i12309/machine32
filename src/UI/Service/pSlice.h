#pragma once
#include <Nextion.h>

#include "../Page.h"

class pSlice: public Page {
    public:

      static pSlice& getInstance() { static pSlice instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { 
        Page::show();
        if (Data::work.profile.valid()) {bListProfile.setText(Data::work.profile.NAME.c_str());}else bListProfile.setText("Выберите профиль!");
      };

      //#####################################################################
      // Text
      NexText tCountPaper  = NexText(25, 5, "tCountPaper");
      NexText t1  = NexText(25, 7, "t1");

      // Button
      NexButton bBack        = NexButton(25, 2, "bBack");
      NexButton bListProfile = NexButton(25, 3, "bListProfile");
      NexButton bSlice       = NexButton(25, 6, "bSlice");
      //#####################################################################

    private:
      NexTouch *nexT[4];

      //#####################################################################

      pSlice() : Page(25, 0, "p5_Slice") {
          nexT[0] = &bBack;
          nexT[1] = &bListProfile;
          nexT[2] = &bSlice;
          nexT[3] = NULL;

          bBack.attachPop(pop_bBack, &bBack);
          bListProfile.attachPop(pop_bListProfile, &bListProfile);
          bSlice.attachPop(pop_bSlice, &bSlice);
      }

      //#####################################################################

      static void pop_bBack(void* ptr);
      static void pop_bListProfile(void* ptr);
      static void pop_bSlice(void* ptr);

};
