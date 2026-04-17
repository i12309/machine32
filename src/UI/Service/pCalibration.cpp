#include "pCalibration.h"
#include "UI/Profile/pProfileList.h"
#include "pService.h"
#include "UI/Main/pINFO.h"
#include "UI/Main/pWAIT.h"

void pCalibration::pop_bBack(void* ptr){
    Log::D(__func__);
    Data::tuning.load(); // возвращаем прошлые настройки  
    App::mode() = State::Mode::SERVICE;
    Data::param.reset();
    App::plan().clear();
    pService::getInstance().show();
}

void pCalibration::pop_bListProfile(void* ptr){
    Log::D(__func__);
    pProfileList& UI = pProfileList::getInstance();

    UI.backPageStatus = Catalog::PageMode::pCalibration;
    UI.show();
    UI.display();
}

void pCalibration::pop_bSave(void* ptr){
    Log::D(__func__);
    pCalibration& UI = pCalibration::getInstance();

    String tDeltaMM      = UI.getText(UI.tDeltaMM,10);
    String tMarkLenghtMM = UI.getText(UI.tMarkLenghtMM,10);
    String tOverMM       = UI.getText(UI.tOverMM,10);
    String tCutCount     = UI.getText(UI.tCutCount,10);
    String tDistanceMM   = UI.getText(UI.tDistanceMM,10);

    Data::tuning.DELTA_mm = atof(tDeltaMM.c_str());
    Data::tuning.MARK_LENGHT_mm = atof(tMarkLenghtMM.c_str());
    Data::tuning.OVER_mm = atof(tOverMM.c_str());
    Data::tuning.DISTANCE_BETWEEN_MARKS_mm = atof(tDistanceMM.c_str());
    Data::tuning.CUT_count = atoi(tCutCount.c_str());

    Data::tuning.save(); // сохраняем 
    App::mode() = State::Mode::SERVICE;
    Data::param.reset();
    App::plan().clear();
    pService::getInstance().show();
}

void pCalibration::pop_bCalibr(void* ptr){
    Log::D(__func__);
    if (!Data::work.profile.valid()) {
        pINFO::showInfo("Профиль бумаги не выбран", "Выберите профиль перед пробой");
        return;
    }
    pCalibration& UI = pCalibration::getInstance();
    
    String tDeltaMM      = UI.getText(UI.tDeltaMM,10);
    String tMarkLenghtMM = UI.getText(UI.tMarkLenghtMM,10);
    String tOverMM       = UI.getText(UI.tOverMM,10);
    String tCutCount     = UI.getText(UI.tCutCount,10);
    String tDistanceMM   = UI.getText(UI.tDistanceMM,10);

    if (
      T::isStringValidFloat(tDeltaMM.c_str())
      && T::isStringValidFloat(tMarkLenghtMM.c_str())
      && T::isStringValidFloat(tOverMM.c_str())
      && T::isStringValidFloat(tDistanceMM.c_str())
      && T::isStringValidInteger(tCutCount.c_str())
      ) 
    {
      Data::tuning.DELTA_mm = atof(tDeltaMM.c_str());
      Data::tuning.MARK_LENGHT_mm = atof(tMarkLenghtMM.c_str());
      Data::tuning.OVER_mm = atof(tOverMM.c_str());
      Data::tuning.DISTANCE_BETWEEN_MARKS_mm = atof(tDistanceMM.c_str());
      Data::tuning.CUT_count = atoi(tCutCount.c_str());

      Data::param.productCutsCount++;
      Data::param.cutsCount = 0;
      if (Data::param.productCutsCount>Data::tuning.CUT_count) Data::param.productCutsCount = 1; // начинаем заново 
      App::plan().clear();

      pWAIT::wait("","Калибровка...","",200, []() {
        if (Data::param.productCutsCount == 1) {
          App::state()->setNexTypeState(State::Type::CALIBRATION);
          App::state()->setFactory(State::Type::CHECK);
        } else {
          App::state()->setFactory(State::Type::CALIBRATION);
        }
      }, false);
    }
}
