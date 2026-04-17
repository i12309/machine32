#include "pStatistics.h"
#include "UI/Main/pMain.h"
#include "pShowStat.h"

  void pStatistics::pop_bBackMainMenu(void* ptr) {
    Log::D(__func__);
    pMain::getInstance().show();
  }

  void pStatistics::pop_bDevice(void* ptr) {
    Log::D(__func__);
    pShowStat::getInstance().showDeviceMotors();
  }

  void pStatistics::pop_bTask(void* ptr) {
    Log::D(__func__);
    pShowStat::getInstance().showTasks();
  }

  void pStatistics::pop_bProfile(void* ptr) {
    Log::D(__func__);
    pShowStat::getInstance().showProfiles();
  }
