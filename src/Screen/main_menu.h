#pragma once

#include "backend_pages/main_menu_base.h"
#include "Screen/load_page.h"

namespace machine32::screen {

// Главное меню. Вся механика разбора Envelope лежит в base-классе,
// здесь — только бизнес-логика.
class MainMenu : public screenui::MainMenuBase {
protected:
    void onShow() override {
        element(cnt_MAIN_MENU).setText("main menu ready");
    }

    void onClickBMainTask() override    { selectSection("task selected"); }
    void onClickBMainProfile() override { selectSection("profile selected"); }
    void onClickBMainNet() override     { selectSection("network selected"); }
    void onClickBMainService() override { selectSection("service selected"); }
    void onClickBMainStats() override   { selectSection("stats selected"); }
    void onClickBMainSupport() override { selectSection("support selected"); }

private:
    void selectSection(const char* status) {
        element(cnt_MAIN_MENU).setText(status);
    }
};

}  // namespace machine32::screen
