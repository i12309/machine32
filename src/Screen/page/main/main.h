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
        
    }

    // Переключает заглушку раздела задания.
    void onClickMainTask() override { }
    // Переключает заглушку раздела профилей.
    void onClickMainProfile() override { }
    // Переключает заглушку раздела сети.
    void onClickMainNet() override { }
    // Переключает заглушку сервисного раздела.
    void onClickMainService() override {  }
    // Переключает заглушку статистики.
    void onClickMainStats() override {  }
    // Переключает заглушку .
    void onClickMainSupport() override {  }

private:

};

}  // namespace machine32::screen
