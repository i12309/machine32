#include "pService.h"
#include "pService2.h"
#include "UI/Main/pMain.h"
#include "pTable.h"
#include "pPaper.h"
#include "pGuillotine.h"
#include "pSlice.h"
#include "pCalibration.h"
#include "UI/Task/pTaskRun.h"

  void pService::pop_bBack(void* ptr) {
    Log::D(__func__);
    App::state()->setFactory(State::Type::IDLE);
    App::plan().clear();
    Stats::getInstance().save();
    pMain::getInstance().show();
  }

  void pService::pop_bNext(void* ptr) {
    Log::D(__func__);
    pService2::getInstance().show();
  }

  void pService::pop_bTable(void* ptr){
    Log::D(__func__);
    pTable::getInstance().show();
  }

  void pService::pop_bPaper(void* ptr){
    Log::D(__func__);
    pPaper::getInstance().show();
  }

  void pService::pop_bGuillotine(void* ptr){
    Log::D(__func__);
    pGuillotine::getInstance().show();
  }

  void pService::pop_bSlice(void* ptr){
    Log::D(__func__);
    pSlice::getInstance().show();
  }

  void pService::pop_bCalibration(void* ptr){
    Log::D(__func__);
    pCalibration::getInstance().show();
  }

  void pService::pop_bProba(void* ptr){
    Log::D(__func__);
    Data::param.frame = 1; // это приладка
    pTaskRun::getInstance().backPageStatus = Catalog::PageMode::pService; // в сервисное меню 
    pTaskRun::getInstance().show();
  }