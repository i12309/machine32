#pragma once
#include <Nextion.h>
#include "NexHardware.h"

#include "UI/Page.h"
#include "Service/WiFiConfig.h"
#include "UI/Main/pMain.h"
#include "version.h"

class pLoad : public Page {
public:
    static constexpr int kProgressCount = 14;

    static pLoad& getInstance() {static pLoad instance; return instance; }

    void loop() override { Page::loop();
    switch(App::state()->type()) {
        case State::Type::IDLE: n_Idle(); break;
    }
    };

    void text(String text) {tStatus.setText(text.c_str());};
    void setProgressColor(int index, uint32_t color) {
        NexText* label = progressLabel(index);
        if (label) {
            label->Set_font_color_pco(color);
        }
    }

    uint32_t getProgressColor(int index) {
        uint32_t color = 0;
        NexText* label = progressLabel(index);
        if (label) {
            label->Get_font_color_pco(&color);
        }
        return color;
    }

    int getHMIVersion() {
        uint32_t value = 0;
        if (verHMI.getValue(&value)) return static_cast<int>(value);

        sendCommand("get verHMI.val");
        if (recvRetNumber(&value)) return static_cast<int>(value);

        sendCommand("get p0.verHMI.val");
        if (recvRetNumber(&value)) return static_cast<int>(value);

        return 0;
    }

    bool checkVersion() {
        tVersion.setText(Version::makeDeviceVersion(getHMIVersion()).c_str());

        //if (VERSION_NEXTION != getHMIVersion()) {tStatus.setText("Версия Nextion не правильная!");return false;}

        tMac.setText(WiFi.macAddress().c_str());

        return true;
    };

      NexText l1       = NexText(0, 2, "l1");
      NexText l2       = NexText(0, 3, "l2");
      NexText l3       = NexText(0, 4, "l3");
      NexText l4       = NexText(0, 5, "l4");
      NexText l5       = NexText(0, 6, "l5");
      NexText l6       = NexText(0, 7, "l6");
      NexText l7       = NexText(0, 8, "l7");
      NexText l8       = NexText(0, 9, "l8");
      NexText l9       = NexText(0, 10, "l9");
      NexText l10      = NexText(0, 11, "l10");
      NexText l11      = NexText(0, 12, "l11");
      NexText l12      = NexText(0, 13, "l12");
      NexText l13      = NexText(0, 14, "l13");
      NexText l14      = NexText(0, 1, "l14");

      NexText tStatus  = NexText(0, 15, "tStatus");
      NexText tVersion = NexText(0, 16, "tVersion");
      NexNumber verHMI = NexNumber(0, 24, "verHMI");
      NexText tMac     = NexText(0, 18, "tMac");


private:
    pLoad() : Page(0, 0, "p0_Load") {}

    NexText* progressLabel(int index) {
        switch (index) {
            case 1: return &l1;
            case 2: return &l2;
            case 3: return &l3;
            case 4: return &l4;
            case 5: return &l5;
            case 6: return &l6;
            case 7: return &l7;
            case 8: return &l8;
            case 9: return &l9;
            case 10: return &l10;
            case 11: return &l11;
            case 12: return &l12;
            case 13: return &l13;
            case 14: return &l14;
            default: return nullptr;
        }
    }

    void n_Idle() {
        if (_setFunc(__func__)){
            pMain::getInstance().show();
          }
    }
};
