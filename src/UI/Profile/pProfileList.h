#pragma once
#include <Nextion.h>

#include "../Page.h"
#include "Catalog.h"

class pProfileList: public Page {
    public:
      static pProfileList& getInstance() { static pProfileList instance; return instance; }
      void loop() override { Page::loop(); nexLoop(nexT); };
      void show() override { Page::show();};

      NexButton *ButtonsPressed[5];
    
      void display();
      Catalog::PageMode backPageStatus; // признак куда переходить по кнопке назад 
    
    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[9];
    
      //#####################################################################
    
      NexButton bBackPage       = NexButton(4,2,"bBackPage");
      NexButton bNextPage       = NexButton(4,3,"bNextPage");
      NexButton bAddItem       = NexButton(4,8,"bAddItem");

      NexButton bItem1       = NexButton(4,4,"bItem1");
      NexButton bItem2       = NexButton(4,5,"bItem2");
      NexButton bItem3       = NexButton(4,6,"bItem3");
      NexButton bItem4       = NexButton(4,7,"bItem4");
      NexButton bItem5       = NexButton(4,16,"bItem5");
    
      //#####################################################################
      pProfileList() : Page(4, 0, "p3_ProfileList") {
        nexT[0] = &bBackPage;
        nexT[1] = &bNextPage;
        nexT[2] = &bAddItem;
        nexT[3] = &bItem1;
        nexT[4] = &bItem2;
        nexT[5] = &bItem3;
        nexT[6] = &bItem4;
        nexT[7] = &bItem5;
        nexT[8] = NULL;  // Завершающий NULL
    
        ButtonsPressed[0] = &bItem1;
        ButtonsPressed[1] = &bItem2;
        ButtonsPressed[2] = &bItem3;
        ButtonsPressed[3] = &bItem4;
        ButtonsPressed[4] = &bItem5;
    
        bBackPage.attachPop(pop_bBackPage,&bBackPage);
        bNextPage.attachPop(pop_bNextPage,&bNextPage);
        bAddItem.attachPop(pop_bAddItem,&bAddItem);
        
        for (int i = 0; i < (sizeof(ButtonsPressed) / sizeof(ButtonsPressed[0])); i++) { ButtonsPressed[i]->attachPop(pop_bPressed, ButtonsPressed[i]); }
      }
    
  //#####################################################################

  static void pop_bBackPage(void* ptr);
  static void pop_bNextPage(void* ptr);
  static void pop_bAddItem(void* ptr);
  static void pop_bPressed(void* ptr);

};
