#include "pTaskRun.h"
#include "UI/Main/pMain.h"
#include "pTaskProcess.h"
#include "pTaskList.h"
#include "../Profile/pProfileList.h"
#include "UI/Service/pService.h"
#include "UI/Main/pINFO.h"
#include "NexHardware.h"

void pTaskRun::pop_bBack(void* ptr) {
    Log::D(__func__);
    auto& UI = pTaskRun::getInstance();
    if (UI.backPageStatus == Catalog::PageMode::pMain) pMain::getInstance().show();
    if (UI.backPageStatus == Catalog::PageMode::pService) pService::getInstance().show();
  }

  void pTaskRun::pop_bStart(void* ptr){
    Log::D(__func__);
    pTaskRun& UI = pTaskRun::getInstance();
    String tCYCLES = UI.getText(UI.tCYCLES,10);
    if (!Data::work.task.valid()) {
      pINFO::showInfo("", "Не выбрано задание", "", nullptr, nullptr, true);
      return;
    }
    if (!Data::work.profile.valid()) {
      pINFO::showInfo("", "Не выбран профиль", "", nullptr, nullptr, true);
      return;
    }
    if (!T::isStringValidInteger(tCYCLES)) {
      pINFO::showInfo("", "Некорректное кол-во листов", "", nullptr, nullptr, true);
      return;
    }
    Data::work.TOTAL_CYCLES = atoi(tCYCLES.c_str());
    if (Data::work.TOTAL_CYCLES <= 0) {
      pINFO::showInfo("", "Кол-во листов должно быть > 0", "", nullptr, nullptr, true);
      return;
    }

    Data::work.TOTAL_CUTS = Data::getCUTs_count();
    if (Data::work.TOTAL_CUTS <= 0) {
      pINFO::showInfo("", "Некорректные параметры задания", "", nullptr, nullptr, true);
      return;
    }

    Log::D(" Start Process ........................");
    App::state()->setNexTypeState(State::Type::PROCESS);// для того что бы после CHECK он смог перейти в PROCESS
    App::state()->setFactory(State::Type::CHECK);
    pTaskProcess::getInstance().show();
  }

  void pTaskRun::pop_bListTask(void* ptr){
    Log::D(__func__);
    pTaskList& list = pTaskList::getInstance();
    list.backPageStatus = Catalog::PageMode::pTaskRun;
    list.show();
    list.display();
  }

  void pTaskRun::pop_bListProfile(void* ptr){
    Log::D(__func__);
    pProfileList& UI = pProfileList::getInstance();
    UI.backPageStatus = Catalog::PageMode::pTaskRun;
    UI.show();
    UI.display();
  }

  void pTaskRun::pop_bMinus(void* ptr){
    Log::D(__func__);
    pTaskRun& UI = pTaskRun::getInstance();
    int count = atoi(UI.getText(UI.tCYCLES,10).c_str())-1;
    if (count < 0) {return;}
    UI.tCYCLES.setText(String(count).c_str());
  }

  void pTaskRun::pop_bPlus(void* ptr){
    Log::D(__func__);
    pTaskRun& UI = pTaskRun::getInstance();
    int count = atoi(UI.getText(UI.tCYCLES,10).c_str())+1;
    if (count > 999) {return;}
    UI.tCYCLES.setText(String(count).c_str());
  }

