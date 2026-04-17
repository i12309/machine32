#pragma once

#include "backend_pages/load_base.h"

namespace machine32::screen {

class MainMenu;  // forward для navigate<MainMenu>()

class LoadPage : public screenui::LoadBase {
protected:
    void onEnter() override {
        // Здесь может быть лёгкая работа: проверки, прогресс-бар.
        // Когда готово — уходим на главное меню.
        // navigate<MainMenu>();  // раскомментируй, когда нужно
    }
};

}  // namespace machine32::screen
