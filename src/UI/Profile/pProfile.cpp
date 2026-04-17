#include "pProfile.h"
#include "pProfileList.h"
#include "UI/Main/pINFO.h"
#include "UI/Main/pWAIT.h"

void pProfile::pop_bNewProfile(void* ptr){
    Log::D(__func__);
    pProfile& UI = pProfile::getInstance();

    String tProfileName = UI.getText(UI.tProfileName,32);
    String tDesc = UI.getText(UI.tDesc,32);
    String tRatioMM = UI.getText(UI.tRatioMM,10);
    String tPaperLenghtMM = UI.getText(UI.tPaperLenghtMM,10);

    tProfileName.trim();
    tDesc.trim();
    tRatioMM.trim();
    tPaperLenghtMM.trim();

    if (T::isStringValidFloat(tRatioMM.c_str()) && T::isStringValidFloat(tPaperLenghtMM.c_str())){

      Data::work.profile.setID(Data::profiles.maxID());
      Data::work.profile.NAME = tProfileName; 
      Data::work.profile.DESC = tDesc;
      Data::work.profile.RATIO_mm = atof(tRatioMM.c_str());
      Data::work.profile.LENGHT_mm = atof(tPaperLenghtMM.c_str());

      Data::profiles.add(Data::work.profile); 

      pProfileList::getInstance().show();
      pProfileList::getInstance().display();
    }
  }

  // редактирование профиля 
  void pProfile::pop_bEditProfile(void* ptr){
    Log::D(__func__);
    pProfile& UI = pProfile::getInstance();

    String tProfileName = UI.getText(UI.tProfileName,32);
    String tDesc = UI.getText(UI.tDesc,32);
    String tRatioMM = UI.getText(UI.tRatioMM,10);
    String tPaperLenghtMM = UI.getText(UI.tPaperLenghtMM,10);

    tProfileName.trim();
    tDesc.trim();
    tRatioMM.trim();
    tPaperLenghtMM.trim();

    if (T::isStringValidFloat(tRatioMM.c_str()) && T::isStringValidFloat(tPaperLenghtMM.c_str()) ){ 

      Data::work.profile.NAME = tProfileName;
      Data::work.profile.DESC = tDesc;
      Data::work.profile.RATIO_mm = atof(tRatioMM.c_str());
      Data::work.profile.LENGHT_mm = atof(tPaperLenghtMM.c_str());

      if (Data::work.profile.valid()) {
        Data::profiles.edit(Data::work.profile);

        App::state()->setFactory(State::Type::IDLE);
        pProfileList::getInstance().show();
        pProfileList::getInstance().display();
      }
    }
  }
  void pProfile::pop_bBack(void* ptr){
    Log::D(__func__);
    App::state()->setFactory(State::Type::IDLE);
    pProfileList::getInstance().show();
    pProfileList::getInstance().display();
  }

  void pProfile::pop_bDel(void* ptr){
    Log::D(__func__);
    if (!Data::work.profile.valid()) {
      Log::D("Нет актуального профиля для удаления");
      return;
    }

    pINFO::showInfo(
    "Удаление профиля","Подтвердите удаление",Data::work.profile.NAME,
    []() {
      Data::profiles.remove(Data::work.profile.ID);
      Data::work.profile.clear();

      pProfileList::getInstance().show();
      pProfileList::getInstance().display();
    },
    nullptr,
    true);
  }

  void pProfile::pop_bProfile(void* ptr){
    Log::D(__func__);
    pINFO::showInfo(
    "Начать профилирование","Положите лист бумаги на стол","для которого надо получить коэф.",
    []() {
      pWAIT::wait("","Профилирование...","",200, []() {
        App::state()->setNexTypeState(State::Type::PROFILING);// для того что бы после CHECK он смог перейти в PROFILING
        App::state()->setFactory(State::Type::CHECK);
      }, false);
    },
    nullptr,
    true);
  }


