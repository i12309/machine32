#pragma once

#include <WiFi.h>

#include "Screen/Screen.h"
#include "Service/WiFiConfig.h"
#include "backend_pages/main_base.h"

namespace machine32::screen {

// Главная страница, которая открывает основные разделы интерфейса.
class Main : public screenui::MainPage<Main> {
public:
    static bool show() {
        return Screen::getInstance().showPage<Main>();
    }

protected:
    // Назначает временные обработчики кликов главного меню.
    void onShow() override {
        btn_MAIN_TASK.onClick = [this] {
            // TODO object-model: открыть Task после миграции страницы.
            Log::D("btn_MAIN_TASK");
        };
        btn_MAIN_PROFILE.onClick = [this] {
            // TODO object-model: открыть Profile после миграции страницы.
        };
        btn_MAIN_NET.onClick = [this] {
            // TODO object-model: открыть Net после миграции страницы.
        };
        btn_MAIN_SERVICE.onClick = [this] {
            // TODO object-model: открыть Service после миграции страницы.
        };
        btn_MAIN_STATS.onClick = [this] {
            // TODO object-model: открыть Stats после миграции страницы.
        };
        btn_MAIN_SUPPORT.onClick = [this] {
            // TODO object-model: открыть Help после миграции страницы.
        };
    }
};

}  // namespace machine32::screen
