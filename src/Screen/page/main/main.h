#pragma once

#include <WiFi.h>

#include "Screen/Screen.h"
#include "Screen/page/main/info.h"
#include "Service/WiFiConfig.h"
#include "backend_pages/main_base.h"

namespace machine32::screen {

// Главная страница, которая открывает основные разделы интерфейса.
class Main : public screenui::MainBase<Main> {
protected:
    // Подготавливает заглушку состояния главного меню.
    void onShow() override {
        element(cnt_MAIN_MENU).setText("main ready");
    }

    // Переключает заглушку раздела задания.
    void onClickMainTask() override { setSection("task"); }
    // Переключает заглушку раздела профилей.
    void onClickMainProfile() override { setSection("profile"); }
    // Переключает заглушку раздела сети.
    void onClickMainNet() override { setSection("network"); }
    // Переключает заглушку сервисного раздела.
    void onClickMainService() override { setSection("service"); }
    // Переключает заглушку статистики.
    void onClickMainStats() override { setSection("stats"); }
    // Переключает заглушку .
    void onClickMainSupport() override { setSection("support"); }

private:
    // Обновляет текстовый индикатор выбранного раздела.
    void setSection(const char* text) {
        element(cnt_MAIN_MENU).setText(text);
    }
};

}  // namespace machine32::screen
