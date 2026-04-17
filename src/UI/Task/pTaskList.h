#pragma once
#include <Nextion.h>

#include "../Page.h"
#include "Catalog.h"

class pTaskList: public Page {
public:

  static pTaskList& getInstance() {static pTaskList instance; return instance; }
  void loop() override { Page::loop(); nexLoop(nexT); };
  void show() override {Page::show();};

  NexButton *ButtonsPressed[5];
  NexButton *ButtonsEdit[5];

  Catalog::PageMode backPageStatus; // признак куда переходить по кнопке назад 


  void display();

private:
  NexTouch *nexT[14];// Массив указателей на объекты NexTouch

  //#####################################################################
  NexButton bBackPage       = NexButton(18,2,"bBackPage");
  NexButton bNextPage       = NexButton(18,3,"bNextPage");
  NexButton bAddItem       = NexButton(18,8,"bAddItem");

  NexButton bItem1       = NexButton(18,4,"bItem1");
  NexButton bItem2       = NexButton(18,5,"bItem2");
  NexButton bItem3       = NexButton(18,6,"bItem3");
  NexButton bItem4       = NexButton(18,7,"bItem4");
  NexButton bItem5       = NexButton(18,16,"bItem5");

  NexButton b1Edit       = NexButton(18,17,"b1Edit");
  NexButton b2Edit       = NexButton(18,18,"b2Edit");
  NexButton b3Edit       = NexButton(18,19,"b3Edit");
  NexButton b4Edit       = NexButton(18,20,"b4Edit");
  NexButton b5Edit       = NexButton(18,21,"b5Edit");


  //#####################################################################
  pTaskList() : Page(18, 0, "p2_TaskList") {
    backPageStatus = Catalog::PageMode::pTaskRun;
    nexT[0] = &bBackPage;
    nexT[1] = &bNextPage;
    nexT[2] = &bAddItem;
    nexT[3] = &bItem1;
    nexT[4] = &bItem2;
    nexT[5] = &bItem3;
    nexT[6] = &bItem4;
    nexT[7] = &bItem5;
    nexT[8] = &b1Edit;
    nexT[9] = &b2Edit;
    nexT[10] = &b3Edit;
    nexT[11] = &b4Edit;
    nexT[12] = &b5Edit;
    nexT[13] = NULL;  // Завершающий NULL

    ButtonsPressed[0] = &bItem1;
    ButtonsPressed[1] = &bItem2;
    ButtonsPressed[2] = &bItem3;
    ButtonsPressed[3] = &bItem4;
    ButtonsPressed[4] = &bItem5;

    ButtonsEdit[0] = &b1Edit;
    ButtonsEdit[1] = &b2Edit;
    ButtonsEdit[2] = &b3Edit;
    ButtonsEdit[3] = &b4Edit;
    ButtonsEdit[4] = &b5Edit;

    bBackPage.attachPop(pop_bBackPage,&bBackPage);
    bNextPage.attachPop(pop_bNextPage,&bNextPage);
    bAddItem.attachPop(pop_bAddItem,&bAddItem);
    
    for (int i = 0; i < (sizeof(ButtonsPressed) / sizeof(ButtonsPressed[0])); i++) { ButtonsPressed[i]->attachPop(pop_bPressed, ButtonsPressed[i]);  }
    for (int i = 0; i < (sizeof(ButtonsEdit) / sizeof(ButtonsEdit[0])); i++) { ButtonsEdit[i]->attachPop(pop_bEdit, ButtonsEdit[i]);  }

  }
    
  //#####################################################################

  static void pop_bBackPage(void* ptr);
  static void pop_bNextPage(void* ptr);
  static void pop_bAddItem(void* ptr);

  static void pop_bPressed(void* ptr);
  static void pop_bEdit(void* ptr);

};
