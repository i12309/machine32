#pragma once

#include "backend_pages/load_base.h"
#include "Screen/Screen.h"
#include <WiFi.h>
#include "version.h"

namespace machine32::screen {

class Main;

class LoadPage : public screenui::LoadBase {
protected:
    void onShow() override {
        Screen& screen = Screen::getInstance();
        const String version = screen.hasDeviceInfo()
            ? String(screen.screenUiVersion())
            : Version::makeDeviceVersion(screen.getScreenVersion());
        element(txt_LOAD_VERSION).setText(version.c_str());
        element(txt_LOAD_MA_CADDRESS).setText(WiFi.macAddress().c_str());

        // Дальше страница живет своим flow.
        // navigate<Main>();  // раскомментируй, когда будет нужен автопереход
    }
};

}  // namespace machine32::screen
