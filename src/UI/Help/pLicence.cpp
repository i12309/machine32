#include "pLicence.h"
#include "UI/Help/pHelp.h"
#include "UI/Main/pWAIT.h"
#include "UI/Main/pINFO.h"

#include "Service/MQTTc.h"
#include "Service/Licence.h"
#include "Service/WiFiConfig.h"

void pLicence::pop_bBack(void* ptr){
    Log::D(__func__);
    pHelp::getInstance().show();
}

void pLicence::pop_bUpdate(void* ptr){
    Log::D(__func__);
    pLicence::getInstance().startLicenceRequest();
}

void pLicence::startLicenceRequest(){
    if (!Core::settings.CONNECT_WIFI || !WiFiConfig::getInstance().isConnect()){
        pINFO::showInfo("Нет соединения","Проверьте Wi-Fi и MQTT","");
        return;
    }

    Licence& licence = Licence::getInstance();
    const bool hadLicence = licence.isLicenceLoaded();
    const String licenceBefore = licence.toJson();

    pWAIT::wait("","Запрашиваем лицензию...","",200, [hadLicence, licenceBefore]() {
        MQTTc::getInstance().licence_request();

        const unsigned long timeout = 8000;
        unsigned long start = millis();
        bool received = false;

        while (millis() - start < timeout) {
            MQTTc::process();
            delay(100);

            if (Licence::getInstance().isLicenceLoaded()) {
                String after = Licence::getInstance().toJson();
                if (!after.isEmpty()) {
                    if (!hadLicence || after != licenceBefore) {
                        received = true;
                        break;
                    }
                    received = true;
                    break;
                }
            }
        }

        pLicence& page = pLicence::getInstance();
        page.show();

        if (received) {
            page.updateLicenceText();
        } else {
            page.tLicence.setText("Лицензия не получена");
        }
    }, false);
}

void pLicence::updateLicenceText(){
    Licence& licence = Licence::getInstance();

    if (licence.isLicenceLoaded()){
        String text = licence.getSummary();
        /*String status = licence.getStatusInfo();
        if (!status.isEmpty()){
            if (!text.isEmpty()){
                text += "\n";
            }
            text += status;
        }*/

        //text.replace("\n", "\r");
        //text.replace("\"", "'");
        if (text.isEmpty()){
            text = "Лицензия загружена";
        }

        tLicence.setText(text.c_str());
    } else {
        tLicence.setText("Лицензия не загружена");
    }
}
