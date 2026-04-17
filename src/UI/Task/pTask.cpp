#include "pTask.h"
#include "pTaskList.h"
#include "UI/Profile/pProfileList.h"
#include "UI/Main/pINFO.h"
#include "Catalog.h"

namespace {

static const Page::TextPicToggleStyle kMarkToggleStyle = {
    Catalog::Color::cyan, // on bco
    Catalog::Color::lighter, // off bco
    Catalog::Color::white, // on pco (white)
    Catalog::Color::black, // off pco
    30,    // on pic
    29     // off pic
};

}

void pTask::pop_bBack(void* ptr){
    pTaskList::getInstance().show();
    pTaskList::getInstance().display();
}

void pTask::pop_bListProfile(void* ptr){
    Log::D(__func__);
    pProfileList& UI = pProfileList::getInstance();

    UI.backPageStatus = Catalog::PageMode::pTask;
    UI.show();
    UI.display();
  }

  // Новый профиль
  void pTask::pop_bNewTask(void* ptr){
    Log::D(__func__);
    pTask& UI = pTask::getInstance();

    String tTaskName = UI.getText(UI.tTaskName,32);
    String tProductMM = UI.getText(UI.tProductMM,10);
    String tOverMM = UI.getText(UI.tOverMM,10);
    String tLastCutMM = UI.getText(UI.tLastCutMM,10);

    String tMarkMM;
    String tFirstCutMM;

    if (!UI.withoutMarkUiState){ // с меткой 
      tMarkMM = UI.getText(UI.tFirstCutMM,10);
      tFirstCutMM = "0";
    }else { // без метки 
      tFirstCutMM = UI.getText(UI.tFirstCutMM,10);
      tMarkMM = "0";
    }

    if (!UI.validateTaskInputs(tTaskName, tProductMM)) return;

    if (T::isStringValidFloat(tProductMM.c_str()) && T::isStringValidFloat(tOverMM.c_str())){
      Data::work.task.setID(Data::tasks.maxID());
      Data::work.task.NAME = tTaskName;
      Data::work.task.FIRST_CUT_mm = atof(tFirstCutMM.c_str());
      Data::work.task.MARK_mm = atof(tMarkMM.c_str());
      Data::work.task.PRODUCT_mm = atof(tProductMM.c_str());
      Data::work.task.OVER_mm = atof(tOverMM.c_str());
      Data::work.task.LASTCUT_mm = atof(tLastCutMM.c_str());
      Data::work.task.MARK = UI.withoutMarkUiState ? 0 : 1;
      Data::work.task.PROFILE_ID = Data::work.profile.ID;

      Data::tasks.add(Data::work.task);
      pTaskList::getInstance().show();
      pTaskList::getInstance().display();
      Data::work.profile.clear();
    }

  }
  
  // редактирование профиля
  void pTask::pop_bEditTask(void* ptr){
    Log::D(__func__);
    pTask& UI = pTask::getInstance();

    String tTaskName = UI.getText(UI.tTaskName,32);
    String tProductMM = UI.getText(UI.tProductMM,10);
    String tOverMM = UI.getText(UI.tOverMM,10);
    String tLastCutMM = UI.getText(UI.tLastCutMM,10);
    String tMarkMM;
    String tFirstCutMM;

    if (!UI.withoutMarkUiState){ // с меткой
      tMarkMM = UI.getText(UI.tFirstCutMM,10);
      tFirstCutMM = "0";
    }else { // без метки
      tFirstCutMM = UI.getText(UI.tFirstCutMM,10);
      tMarkMM = "0";
    }

    if (!UI.validateTaskInputs(tTaskName, tProductMM)) return;

    if (T::isStringValidFloat(tProductMM.c_str()) && T::isStringValidFloat(tOverMM.c_str()) && T::isStringValidFloat(tLastCutMM.c_str())){

      Data::work.task.NAME = tTaskName;
      Data::work.task.FIRST_CUT_mm = atof(tFirstCutMM.c_str());
      Data::work.task.MARK_mm = atof(tMarkMM.c_str());
      Data::work.task.PRODUCT_mm = atof(tProductMM.c_str());
      Data::work.task.OVER_mm = atof(tOverMM.c_str());
      Data::work.task.LASTCUT_mm = atof(tLastCutMM.c_str());
      Data::work.task.MARK = UI.withoutMarkUiState ? 0 : 1;
      Data::work.task.PROFILE_ID = Data::work.profile.ID;

      //Data::work.task.print();
      if (Data::work.task.valid()) {
        Data::tasks.edit(Data::work.task);
        Data::work.task.clear();
        pTaskList::getInstance().show();
        pTaskList::getInstance().display();
      }
    }
  }

void pTask::pop_bDel(void* ptr){
    Log::D(__func__);
    if (!Data::work.task.valid()) {
      Log::D("Нет актуального задания для удаления");
      return;
    }

    pINFO::showInfo(
    "Удаление задания","Подтвердите удаление",Data::work.task.NAME,
    []() {
      Data::tasks.remove(Data::work.task.ID);
      Data::work.task.clear();

      pTaskList::getInstance().show();
      pTaskList::getInstance().display();
    },
    nullptr,
    true);
}

bool pTask::validateTaskInputs(const String& name, const String& product) const {
    String trimmedName = name;
    trimmedName.trim();
    if (trimmedName.isEmpty()) {
      pINFO::showInfo("Задание", "Не заполнено название", "", nullptr, nullptr, true);
      return false;
    }
    if (!Data::work.profile.valid()) {
      pINFO::showInfo("Задание", "Не выбран профиль", "", nullptr, nullptr, true);
      return false;
    }
    float productValue = atof(product.c_str());
    if (!T::isStringValidFloat(product.c_str()) || productValue <= 0.0f) {
      pINFO::showInfo("Задание", "Изделие должно быть > 0", "", nullptr, nullptr, true);
      return false;
    }
    return true;
}

void pTask::setMarkToggle(bool withoutMark){
    withoutMarkUiState = withoutMark;
    Page::setTextPicToggle(tMark, rAutoUpdate, withoutMark, kMarkToggleStyle);
    showFirstCut();
}

void pTask::showFirstCut(){
    if (!withoutMarkUiState){ // с меткой
      tMark.setText("Метка  ");
      tMarkDesc.setText("ширина метки");
      tFirstCutMM.setText(String(Data::work.task.MARK_mm).c_str());
      tFirstCutMM.Set_background_color_bco(Catalog::Color::lighter);
      tFirstCutMM.Set_font_color_pco(Catalog::Color::black);
    }else { // без метки
      tMark.setText("Первый\r\nрез");
      tMarkDesc.setText("рез от края");
      tFirstCutMM.setText(String(Data::work.task.FIRST_CUT_mm).c_str());
      tFirstCutMM.Set_background_color_bco(Catalog::Color::cyan);
      tFirstCutMM.Set_font_color_pco(Catalog::Color::white);
    }
}

void pTask::pop_tMark(void* ptr){
    Log::D(__func__);
    pTask& UI = pTask::getInstance();
    UI.withoutMarkUiState = Page::toggleTextPicByColor(UI.tMark, UI.rAutoUpdate, kMarkToggleStyle);
    UI.showFirstCut();
}

