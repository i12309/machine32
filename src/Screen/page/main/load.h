#pragma once

#include <WiFi.h>

#include "Screen/Screen.h"
#include "Screen/page/main/main.h"
#include "State/State.h"
#include "backend_pages/load_base.h"
#include "version.h"

namespace machine32::screen {

class Load : public screenui::LoadBase {
protected:
    void onShow() override {
        _openedMain = false;

        Screen& screen = Screen::getInstance();
        const String version = screen.hasDeviceInfo()
            ? String(screen.screenUiVersion())
            : Version::makeDeviceVersion(screen.getScreenVersion());
        element(txt_LOAD_VERSION).setText(version.c_str());
        element(txt_LOAD_MA_CADDRESS).setText(WiFi.macAddress().c_str());
    }

    void onTick() override {
        State* currentState = App::state();
        if (currentState == nullptr || _openedMain) {
            return;
        }

        if (currentState->type() == State::Type::IDLE) {
            _openedMain = true;
            navigate<Main>();
        }
    }

private:
    bool _openedMain = false;
};

}  // namespace machine32::screen
