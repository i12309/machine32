#pragma once

#include "backend_pages/main_base.h"

namespace machine32::screen {

class Main : public screenui::MainBase {
protected:
    void onShow() override {
        element(cnt_MAIN_MENU).setText("main ready");
    }

    void onClickMainTask() override { setSection("task"); }
    void onClickMainProfile() override { setSection("profile"); }
    void onClickMainNet() override { setSection("network"); }
    void onClickMainService() override { setSection("service"); }
    void onClickMainStats() override { setSection("stats"); }
    void onClickMainSupport() override { setSection("support"); }

private:
    void setSection(const char* text) {
        element(cnt_MAIN_MENU).setText(text);
    }
};

}  // namespace machine32::screen
