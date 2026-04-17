#include "pHelp.h"
#include "UI/Main/pMain.h"
#include "UI/Help/pLicence.h"
#include "UI/Help/pUpdate.h"
#include "UI/Help/pDevice.h"
#include "UI/Main/pINPUT.h"

  void pHelp::pop_bBack(void* ptr) {
    Log::D(__func__);
    pMain::getInstance().show();
  }

  void pHelp::pop_bTO(void* ptr) {
    Log::D(__func__);

    // если это DEV машина пропускаем 
    if (Core::config.group == "DEV") {pDevice::getInstance().show(); return;}

    pINPUT::showInput(
    "Устройство","Настройка устройства","","",
    [](const String& input) {if (input == WiFiConfig::getInstance().mac_xx()) pDevice::getInstance().show();},
    []() {pINPUT::getInstance().back();},
    1, // Клавиатура  
    false);
  }

  void pHelp::pop_bUpdate(void* ptr) {
    Log::D(__func__);
    pUpdate::getInstance().show();
  }

  void pHelp::pop_bLicence(void* ptr) {
    Log::D(__func__);
    pLicence::getInstance().show();
  }
