#include "pUpdate.h"
#include "UI/Main/pWAIT.h"
#include "UI/Help/pHelp.h"

void pUpdate::pop_bBack(void* ptr){
    Log::D(__func__);
    pUpdate::getInstance().getAutoUpdateUI();
    Core::config.save();
    pHelp::getInstance().show();
}

void pUpdate::pop_bUpdateESP(void* ptr){
    Log::D(__func__);
    if (Core::settings.CONNECT_WIFI && WiFiConfig::getInstance().isConnect()){
        if (Core::settings.UPDATE) {
            int level = ESPUpdate::getInstance().checkForUpdate();
            if (level > 0) {
                pWAIT::wait("","Обновление","",0,nullptr,false);
                ESPUpdate::getInstance().FirmwareUpdate(level, [](int percent) {
                    String text = "Обновление: " + String(percent) + "%";
                    pWAIT::getInstance().tText2.setText(text.c_str());
                });
            }
        }
    }
}

void pUpdate::pop_bUpdateTFT(void* ptr){
    Log::D(__func__);
    if (Core::settings.CONNECT_WIFI && WiFiConfig::getInstance().isConnect()){
        if (Core::settings.TFT_UPDATE) {
            int level = NexUpdate::getInstance().checkForUpdate();
            if (level > 0) {
                pWAIT::wait("","Обновление интерфейса","",0,nullptr,false);
                NexUpdate::getInstance().upload(level, true);
            }
        }
    }
}

void pUpdate::pop_tAutoUpdate(void* ptr){
    Log::D(__func__);
    Core::settings.AUTO_UPDATE = !Core::settings.AUTO_UPDATE;
    pUpdate::getInstance().setAutoUpdateUI();
}
