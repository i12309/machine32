#pragma once
#include <Nextion.h>

#include "../Page.h"

class pCalibration: public Page {
    public:

      static pCalibration& getInstance() { static pCalibration instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { 
        Page::show();

        if (Data::work.profile.valid()) {bListProfile.setText(Data::work.profile.NAME.c_str());}else bListProfile.setText("Выберите профиль!");

        tDeltaMM.setText(((String)Data::tuning.DELTA_mm).c_str());
        tMarkLenghtMM.setText(((String)Data::tuning.MARK_LENGHT_mm).c_str());
        tOverMM.setText(((String)Data::tuning.OVER_mm).c_str());
        tCutCount.setText(((String)Data::tuning.CUT_count).c_str());
        tDistanceMM.setText(((String)Data::tuning.DISTANCE_BETWEEN_MARKS_mm).c_str());
      };

      //#####################################################################
      // Text
      NexText tDeltaMM      = NexText(22, 6, "tDeltaMM");
      NexText tMarkLenghtMM = NexText(22, 9, "tMarkLenghtMM");
      NexText tOverMM       = NexText(22, 11, "tOverMM");
      NexText tCutCount     = NexText(22, 13, "tCutCount");
      NexText tDistanceMM   = NexText(22, 15, "tDistanceMM");

      // Button
      NexButton bSave         = NexButton(22, 2, "bSave");
      NexButton bBack         = NexButton(22, 3, "bBack");
      NexButton bListProfile  = NexButton(22, 4, "bListProfile");
      NexButton bCalibr       = NexButton(22, 7, "bCalibr");
      //#####################################################################

    private:
      NexTouch *nexT[5];

      //#####################################################################

      pCalibration() : Page(22, 0, "p5_Calibration") {
          nexT[0] = &bSave;
          nexT[1] = &bBack;
          nexT[2] = &bListProfile;
          nexT[3] = &bCalibr;
          nexT[4] = NULL;

          bSave.attachPop(pop_bSave, &bSave);
          bBack.attachPop(pop_bBack, &bBack);
          bListProfile.attachPop(pop_bListProfile, &bListProfile);
          bCalibr.attachPop(pop_bCalibr, &bCalibr);
      }

      //#####################################################################

      static void pop_bSave(void* ptr);
      static void pop_bBack(void* ptr);
      static void pop_bListProfile(void* ptr);
      static void pop_bCalibr(void* ptr);

};
