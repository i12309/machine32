#pragma once
#include <Nextion.h>
#include "Catalog.h"

#include "Service/WiFiConfig.h"
#include "Service/HServer.h"
#include "Service/Licence.h"
#include "UI/Page.h"
#include "UI/Main/pINFO.h"

class pMain: public Page {
public:
  //String vTFT;

  static pMain& getInstance() {static pMain instance; return instance; }

  void loop() override { 
    Page::loop(); 
    nexLoop(nexT);
    switch(App::state()->type()) {
      case State::Type::IDLE: n_Idle(); break;
    } 
  };

  void show() override { //Log::D("show - pMain"); 
    Page::show();
    Data::work.clear();

    tRSSIvalue.setText(WiFiConfig::getInstance().getNetInfo().c_str());
    tNetvalue.setText(String(WiFiConfig::getInstance().getRSSI()).c_str());

  };

private:
  // Массив указателей на объекты NexTouch
  NexTouch *nexT[8];

  //#####################################################################

  NexText tStatusPanel     = NexText(1, 7, "tStatusPanel");
  NexText tRSSIvalue            = NexText(1, 8, "tRSSIvalue");
  NexText tNetvalue            = NexText(1, 14, "tNetvalue");

  //#####################################################################

  NexButton bProgram       = NexButton(1,7,"bProgram");
  NexButton bService       = NexButton(1,10,"bService");
  NexButton bProfile       = NexButton(1,8,"bProfile");

  NexButton bSettings       = NexButton(1,9,"bSettings");
  NexButton bStatistic       = NexButton(1,11,"bStatistic");
  NexButton bHelp       = NexButton(1,12,"bHelp");

  NexText bNetBackground       = NexText(1,5,"t5");

  //#####################################################################
  pMain() : Page(1, 0, "p1_MainMenu") { 
      nexT[0] = &bProgram;
      nexT[1] = &bService;
      nexT[2] = &bProfile;
      nexT[3] = &bSettings;
      nexT[4] = &bStatistic;
      nexT[5] = &bHelp;
      nexT[6] = &tNetvalue;
      nexT[7] = NULL;  // Завершающий NULL
      
      bProgram.attachPop(pop_bProgram, &bProgram);
      bService.attachPop(pop_bService, &bService);
      bProfile.attachPop(pop_bProfile, &bProfile);

      // wifi 
      bSettings.attachPop(pop_bSettings, &bSettings); tNetvalue.attachPop(pop_bSettings, &tNetvalue);

      bStatistic.attachPop(pop_bStatistic, &bStatistic);
      bHelp.attachPop(pop_bHelp, &bHelp);
      
  }

  //#####################################################################

  static void pop_bProgram(void* ptr);
  static void pop_bService(void* ptr);
  static void pop_bProfile(void* ptr);
  static void pop_bSettings(void* ptr);
  static void pop_bStatistic(void* ptr);
  static void pop_bHelp(void* ptr);

  //#####################################################################

  void n_Idle() {

    if (_setFunc(__func__)){
      if (!Licence::getInstance().isValid()) { 
          pINFO::showInfo("Лицензия не верная", "Работа не возможна!");
      }
    }

    // Обновляем инфу про wifi каждые 5 сек
    static unsigned long lastUpdateTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= 5000) {
        lastUpdateTime = currentTime;
        tRSSIvalue.setText(WiFiConfig::getInstance().getNetInfo().c_str());
        tNetvalue.setText(String(WiFiConfig::getInstance().getRSSI()).c_str());

        if (WiFi.RSSI() == 0) bNetBackground.Set_background_color_bco(Catalog::Color::mediumGrey); else bNetBackground.Set_background_color_bco(Catalog::Color::orange);

    }

    // проверяем нажата ли кнопка 
    checkButton("START", pop_bProgram);
  }
  
};

