#include "pMain.h"
#include "UI/Task/pTaskRun.h"
#include "UI/Service/pService.h"
#include "UI/Profile/pProfileList.h"
#include "UI/Wifi/pWiFi.h"
#include "UI/Statistics/pStatistics.h"
#include "UI/Help/pHelp.h"

void pMain::pop_bProgram(void* ptr){
    Log::D(__func__);
    if (Data::work.task.valid()) { Data::profiles.getByID(Data::work.task.PROFILE_ID,Data::work.profile); } 
    Data::param.frame = 0; // это не приладка а обыная работа 
    pTaskRun::getInstance().backPageStatus = Catalog::PageMode::pMain; // в главное меню 
    pTaskRun::getInstance().show();
  }

void pMain::pop_bService(void* ptr) {
    Log::D(__func__);
    pService::getInstance().show();
  }

void pMain::pop_bProfile(void* ptr) {
    Log::D(__func__);
    pProfileList& UI = pProfileList::getInstance();
    UI.backPageStatus = Catalog::PageMode::pMain;
    UI.show();
    UI.display(); 
  }

  void pMain::pop_bSettings(void* ptr){
    Log::D(__func__);
    pWiFi::getInstance().show();
  }

  void pMain::pop_bStatistic(void* ptr){
    Log::D(__func__);
    pStatistics::getInstance().show();
  }

  void pMain::pop_bHelp(void* ptr){
    Log::D(__func__);
    pHelp::getInstance().show();
  }