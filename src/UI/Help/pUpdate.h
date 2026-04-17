#pragma once
#include <Nextion.h>

#include "Service/WiFiConfig.h"
#include "Service/ESPUpdate.h"
#include "Service/NexUpdate.h"
#include "UI/Main/pLoad.h"
#include "Catalog.h"
#include "version.h"

#include "../Page.h"

class pUpdate: public Page {
    public:

      static pUpdate& getInstance() { static pUpdate instance; return instance; }

      void loop() override { Page::loop(); nexLoop(nexT); };

      void show() override { Page::show();
        setAutoUpdateUI();
        String versionESP = String(APP_VERSION);
        String versionTFT = String(NexUpdate::getInstance().getCurrentVersion());
        String fullVersion = Version::makeDeviceVersion(NexUpdate::getInstance().getCurrentVersion());

        if (ESPUpdate::getInstance().checkForUpdate() > 0) versionESP = String(ESPUpdate::getInstance().getNewAppVersion())+ " новая";

        if (NexUpdate::getInstance().checkForUpdate() > 0) versionTFT = String(NexUpdate::getInstance().getNewAppVersion())+ " новая";

        tVersionESP.setText(versionESP.c_str());
        tVersionTFT.setText(versionTFT.c_str());
        tFullVersion.setText(("   Версия: "+fullVersion).c_str());
      };

      void setAutoUpdateUI(){
        if (Core::settings.AUTO_UPDATE){
          tAutoUpdate.Set_background_color_bco(Catalog::Color::yellow); // Желтый
          tAutoUpdate.Set_font_color_pco(Catalog::Color::white); // белый
          rAutoUpdate.Set_background_image_pic(Catalog::UI::sw_pic2);
        }
        else {
          tAutoUpdate.Set_background_color_bco(Catalog::Color::lightGrey); // Серый
          tAutoUpdate.Set_font_color_pco(Catalog::Color::black); // темный
          rAutoUpdate.Set_background_image_pic(Catalog::UI::sw_pic1);
        }
      }

      void getAutoUpdateUI(){
        uint32_t bco;
        uint32_t status = tAutoUpdate.Get_background_color_bco(&bco);;
        if (bco == Catalog::Color::yellow){ // Желтый
          Core::settings.AUTO_UPDATE = true;
        }
        else Core::settings.AUTO_UPDATE = false;
      }

    private:
      // Массив указателей на объекты NexTouch
      NexTouch *nexT[5];

      //#####################################################################

      NexText tFullVersion    = NexText(21, 4, "tFullVersion");
      NexText tVersionESP     = NexText(21, 7, "tVersionESP");
      NexText tVersionTFT     = NexText(21, 11, "tVersionTFT");

      NexText tAutoUpdate = NexText(21,3,"tAutoUpdate");
      NexPicture rAutoUpdate = NexPicture(21,21,"rAutoUpdate");

      //#####################################################################

      NexButton bUpdateESP       = NexButton(21,9,"bUpdateESP");
      NexButton bUpdateTFT       = NexButton(21,12,"bUpdateTFT");
      NexButton bBack       = NexButton(21,7,"bBack");

      //#####################################################################
      pUpdate() : Page(21, 0, "p4_Update") {
          nexT[0] = &bUpdateESP;
          nexT[1] = &bUpdateTFT;
          nexT[2] = &bBack;
          nexT[3] = &tAutoUpdate;
          nexT[4] = NULL;

          bBack.attachPop(pop_bBack, &bBack);
          bUpdateESP.attachPop(pop_bUpdateESP, &bUpdateESP);
          bUpdateTFT.attachPop(pop_bUpdateTFT, &bUpdateTFT);
          tAutoUpdate.attachPop(pop_tAutoUpdate, &tAutoUpdate);
      }

  //#####################################################################

  static void pop_bBack(void* ptr);
  static void pop_bUpdateESP(void* ptr);
  static void pop_bUpdateTFT(void* ptr);
  static void pop_tAutoUpdate(void* ptr);

};
