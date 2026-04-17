#include "pProfileList.h"
#include "UI/Main/pMain.h"
#include "UI/Task/pTaskRun.h"
#include "UI/Task/pTask.h"
#include "pProfile.h"
#include "Catalog.h"
#include "UI/Service/pCalibration.h"
#include "UI/Service/pSlice.h"

void pProfileList::display() {
    pProfileList& UI = pProfileList::getInstance();

    Data::profiles.countPages();
    Page::setVisible(UI.bNextPage, (Data::profiles.curentPage + 1) < Data::profiles.totalPages);
    const int ITEMS_COUNT = sizeof(UI.ButtonsPressed) / sizeof(UI.ButtonsPressed[0]);
    // Очистка всех элементов
    for (int i = 0; i < ITEMS_COUNT; i++) {UI.ButtonsPressed[i]->setText("");}

    // Получение данных страницы
    int itemCount;
    Data::profiles.ListItems = Data::profiles.getPage(itemCount);

    // Обновление видимых элементов
    char txt[32];
    for (int i = 0; i < itemCount && i < ITEMS_COUNT; i++) {
        // порядковые номера и названия 
        snprintf(txt, sizeof(txt), "  %d. %s",Data::profiles.curentPage * Data::profiles.pageSize + i + 1, Data::profiles.ListItems[i].NAME.c_str());
        // Обновление кнопки с названием задачи
        UI.ButtonsPressed[i]->setText(txt);
    }
  }

void pProfileList::pop_bBackPage(void* ptr){
    Log::D(__func__);
    pProfileList& UI = pProfileList::getInstance();
    if (Data::profiles.curentPage==0){
      if (UI.backPageStatus==Catalog::PageMode::pMain) pMain::getInstance().show();
      if (UI.backPageStatus==Catalog::PageMode::pTaskRun) pTaskRun::getInstance().show();
      if (UI.backPageStatus==Catalog::PageMode::pTask) pTask::getInstance().show();
      if (UI.backPageStatus==Catalog::PageMode::pCalibration) pCalibration::getInstance().show();
      if (UI.backPageStatus==Catalog::PageMode::pSlice) pSlice::getInstance().show();
    }
    else {
      Data::profiles.curentPage--;
      UI.display();
  }
}

void pProfileList::pop_bNextPage(void* ptr){
    Log::D(__func__);
    pProfileList& UI = pProfileList::getInstance();

    if ((Data::profiles.curentPage+1)<Data::profiles.totalPages){
      Data::profiles.curentPage++;
      UI.display();
    }
}

void pProfileList::pop_bAddItem(void* ptr){
    Log::D(__func__);
    pProfile& profile = pProfile::getInstance();
    
    profile.setFormMode(Catalog::FormMode::CREATE);
    profile.show();
    profile.tProfileName.setText("");
    profile.tDesc.setText("");
    profile.tRatioMM.setText("0");
    profile.tPaperLenghtMM.setText("0");
    profile.bSave.attachPop(profile.pop_bNewProfile,&profile.bSave);
    pProfileList::getInstance().display();
}

void pProfileList::pop_bPressed(void* ptr){
    Log::D(__func__);
    pProfileList& UI = pProfileList::getInstance();

    // Количество кнопок
    const int BUTTON_COUNT = sizeof(UI.ButtonsPressed) / sizeof(UI.ButtonsPressed[0]);
    // Определяем, какая кнопка была нажата
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (ptr == UI.ButtonsPressed[i]) {
            // Получаем профиль по текущей странице и позиции кнопки
            Data::profiles.getByPage(i,Data::work.profile); 
            if (Data::work.profile.valid()) {
              if (UI.backPageStatus==Catalog::PageMode::pMain) {  // открыли через pMain
                  pProfile& profile = pProfile::getInstance();
                  profile.setFormMode(Catalog::FormMode::EDIT);
                  profile.show();
                  profile.tProfileName.setText(Data::work.profile.NAME.c_str());
                  profile.tDesc.setText(Data::work.profile.DESC.c_str());
                  profile.tRatioMM.setText(String(Data::work.profile.RATIO_mm, 2).c_str());
                  profile.tPaperLenghtMM.setText(String(Data::work.profile.LENGHT_mm, 2).c_str());
                  profile.bSave.attachPop(profile.pop_bEditProfile,&profile.bSave);
            }
                if (UI.backPageStatus==Catalog::PageMode::pTaskRun){ // значит окно открыли через pTaskRun
                  pTaskRun::getInstance().show();
                }
                if (UI.backPageStatus==Catalog::PageMode::pTask){ // значит окно открыли через pTask 
                  pTask::getInstance().show();
                  pTask::getInstance().bListProfile.setText(Data::work.profile.NAME.c_str());
                }
                if (UI.backPageStatus==Catalog::PageMode::pCalibration){ // значит окно открыли через pCalibration
                  pCalibration::getInstance().show();
                  pCalibration::getInstance().bListProfile.setText(Data::work.profile.NAME.c_str());
                }
                if (UI.backPageStatus==Catalog::PageMode::pSlice){ // значит окно открыли через pSlice
                  pSlice::getInstance().show();
                  pSlice::getInstance().bListProfile.setText(Data::work.profile.NAME.c_str());
                }
            }
            break;
        }
    }
}
