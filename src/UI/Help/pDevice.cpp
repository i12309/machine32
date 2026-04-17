#include "pDevice.h"
#include <Arduino.h>
#include "pHelp.h"
#include "UI/Main/pINPUT.h"
#include "PinTest.h"
#include "pParams.h"
#include "Service/PinTester.h"
#include "UI/Main/pINFO.h"
#include "Service/WiFiConfig.h"
#include "Service/Stats.h"
#include "Core.h"

void pDevice::pop_bBack(void* ptr) {
  Log::D(__func__);
  pHelp::getInstance().show();
}

void pDevice::pop_bParams(void* ptr){
  Log::D(__func__);
  pParams::getInstance().show();
}

void pDevice::pop_bPinTest(void* ptr){
  Log::D(__func__);
  PinTest::getInstance().show();
  PinTester::initTestMode();
}

void pDevice::pop_bPressure(void* ptr){
  Log::D(__func__);
  pINFO::showInfo("", "", "");
  App::state()->setFactory(State::Type::PRESSURE);
}

void pDevice::pop_bNULL(void* ptr){
  Log::D(__func__);
  pINFO::showInfo(
    "Удалить все данные?",
    "Wi-Fi, статистика, настройки",
    "Нажмите OK для продолжения",
    []() {
      pINFO::showInfo(
        "Удалить Wi-Fi?",
        "Стереть сохраненные Wi-Fi сети?",
        "",
        []() {
          Log::D("Удаление данных с очисткой Wi-Fi");
          bool wifiOk = WiFiConfig::getInstance().clearAll();
          bool statsOk = Stats::getInstance().clearAll();
          bool configOk = FileSystem::deleteFile(CONFIG_PATH.c_str());
          bool dataOk = FileSystem::deleteFile(DATA_PATH.c_str());
          Log::D("Удалено: wifi=%d stats=%d config=%d data=%d", wifiOk, statsOk, configOk, dataOk);
          delay(200);
          ESP.restart();
        },
        []() {
          Log::D("Удаление данных без очистки Wi-Fi");
          bool statsOk = Stats::getInstance().clearAll();
          bool configOk = FileSystem::deleteFile(CONFIG_PATH.c_str());
          bool dataOk = FileSystem::deleteFile(DATA_PATH.c_str());
          Log::D("Удалено: wifi=skip stats=%d config=%d data=%d", statsOk, configOk, dataOk);
          delay(200);
          ESP.restart();
        },
        true
      );
    },
    nullptr,
    true
  );
}

void pDevice::pop_b5(void* ptr){Log::D(__func__);}
void pDevice::pop_b6(void* ptr){Log::D(__func__);}
