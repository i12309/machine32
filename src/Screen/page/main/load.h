#pragma once

#include <WiFi.h>

#include "Screen/Screen.h"
#include "Screen/page/main/main.h"
#include "State/State.h"
#include "backend_pages/load_base.h"
#include "version.h"

namespace machine32::screen {

// Страница загрузки, которая показывает версию и MAC до перехода дальше.
class Load : public screenui::LoadBase<Load> {
protected:
    // Подготавливает отображаемые сервисные данные страницы загрузки.
    void onShow() override {
        _openedMain = false;

        Screen& screen = Screen::getInstance();
        const String version = screen.hasDeviceInfo()
            ? String(screen.screenUiVersion())
            : Version::makeDeviceVersion(screen.getScreenVersion());
        element(txt_LOAD_VERSION).setText(version.c_str());
        element(txt_LOAD_MA_CADDRESS).setText(WiFi.macAddress().c_str());
    }

    // После перехода состояния в IDLE открывает главный экран.
    void onTick() override {
        State* currentState = App::state();
        if (currentState == nullptr || _openedMain) {
            return;
        }

        if (currentState->type() == State::Type::IDLE) {
            _openedMain = true;
            Main::show();
        }
    }

private:
    bool _openedMain = false;
};

}  // namespace machine32::screen
