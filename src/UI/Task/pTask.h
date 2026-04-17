#pragma once
#include <Nextion.h>

#include "../Page.h"
#include "Catalog.h"

class pTask: public Page {
    public:
    
      static pTask& getInstance() { static pTask instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override {Page::show(); applyFormMode();};
    
      //#####################################################################
      NexText tTaskName       = NexText(16, 9, "tTaskName");
      NexText tProductMM       = NexText(16, 7, "tProductMM");
      NexText tOverMM       = NexText(16, 11, "tOverMM");
      NexText tLastCutMM       = NexText(16, 5, "tLastCutMM");

      NexText tMark       = NexText(16, 13, "tMark");
      NexText tFirstCutMM       = NexText(16, 14, "tFirstCutMM");
      NexText tMarkDesc  = NexText(16, 20, "tMarkDesc");
      NexPicture rAutoUpdate = NexPicture(16, 21, "rAutoUpdate");

      //#####################################################################
      NexButton bListProfile       = NexButton(16,12,"bListProfile");
      NexButton bBack       = NexButton(16,3,"bBack");
      NexButton bSave       = NexButton(16,2,"bSave");
      NexButton bDel           = NexButton(16, 17, "bDel");
      //#####################################################################

  // Варинаты для кнопки bSave
  static void pop_bNewTask(void* ptr);
  static void pop_bEditTask(void* ptr);
  
  void setFormMode(Catalog::FormMode mode) {
    formMode = mode;
  }

  void showFirstCut();
  void setMarkToggle(bool withoutMark);
  bool validateTaskInputs(const String& name, const String& product) const;

    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[6];
      Catalog::FormMode formMode = Catalog::FormMode::VIEW;
      bool withoutMarkUiState = false;
      
      pTask() : Page(16, 0, "p2_Task") {
          nexT[0] = &bListProfile;
          nexT[1] = &bBack;
          nexT[2] = &bSave;
          nexT[3] = &bDel;
          nexT[4] = &tMark;
          nexT[5] = NULL;  // Завершающий NULL
    
          bListProfile.attachPop(pop_bListProfile, &bListProfile);
          bBack.attachPop(pop_bBack, &bBack);
          bDel.attachPop(pop_bDel, &bDel);
          tMark.attachPop(pop_tMark, &tMark);
      }
    
  //#####################################################################

  static void pop_bListProfile(void* ptr);
  static void pop_bBack(void* ptr);
  static void pop_bDel(void* ptr);
  
  void applyFormMode() {
    bool showDel = (formMode == Catalog::FormMode::EDIT);
    Page::setVisible(bDel, showDel);
  }
  static void pop_tMark(void* ptr);

};
