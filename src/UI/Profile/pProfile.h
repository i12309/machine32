#pragma once
#include <Nextion.h>

#include "../Page.h"
#include "Catalog.h"

class pProfile: public Page {
  public:
    
    static pProfile& getInstance() { static pProfile instance; return instance; }
    void loop() override { Page::loop(); nexLoop(nexT); 
      switch(App::state()->type()) {
        case State::Type::FINISH: n_FinishCycle(); break;
      } 
    };
    void show() override { Page::show(); applyFormMode(); };
  
    //##################################################################### 
  
      // Text
      NexText tProfileName   = NexText(17, 9, "tProfileName");
      NexText tRatioMM       = NexText(17, 11, "tRatioMM");
      NexText tDesc          = NexText(17, 14, "tDesc");
      NexText tPaperLenghtMM = NexText(17, 16, "tPaperLenghtMM");

      // Button
      NexButton bBack   = NexButton(17, 6, "bBack");
      NexButton bSave          = NexButton(17, 7, "bSave");
      NexButton bProfile       = NexButton(17, 12, "bProfile");
      NexButton bDel           = NexButton(17, 18, "bDel");
  
    //#####################################################################
      
  static void pop_bNewProfile(void* ptr);
  static void pop_bEditProfile(void* ptr);
  
  void setFormMode(Catalog::FormMode mode) {
    formMode = mode;
  }
  
  private:
    // Массив указателей на объекты NexTouch
    NexTouch *nexT[5];
    Catalog::FormMode formMode = Catalog::FormMode::VIEW;

    pProfile() : Page(17, 0, "p3_Profile") {
        nexT[0] = &bBack;
        nexT[1] = &bSave;
        nexT[2] = &bProfile;
        nexT[3] = &bDel;
        nexT[4] = NULL;
  
        bSave.attachPop(nullptr,&bSave);

        bBack.attachPop(pop_bBack,&bBack);
        bProfile.attachPop(pop_bProfile, &bProfile);
        bDel.attachPop(pop_bDel, &bDel);
    }
    
  //#####################################################################

  static void pop_bBack(void* ptr);
  static void pop_bProfile(void* ptr);
  static void pop_bDel(void* ptr);

  void applyFormMode() {
    bool showDel = (formMode == Catalog::FormMode::EDIT);
    Page::setVisible(bDel, showDel);
  }

  //#####################################################################

  void n_FinishCycle() {
    if (_setFunc(__func__)){ // только один раз
      tRatioMM.setText(((String)Data::work.profile.RATIO_mm).c_str()); 
    }
  };

};
