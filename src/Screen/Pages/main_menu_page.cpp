#include "Screen/Pages/main_menu_page.h"

#include "Screen/runtime.h"

namespace machine32::screen {

void screen_page_main_menu::on_enter(runtime& ui) {
    // Это демонстрационный пример.
    // Пока ScreenUI ещё не завершён, страница просто показывает, что backend
    // успешно вошёл в главное меню и готов принимать пользовательские события.
    ui.text(SCREEN32_ELEMENT_ID_C_MAIN_MENU, "Главное меню готово");
}

bool screen_page_main_menu::on_button(runtime& ui, uint32_t element_id) {
    switch (element_id) {
        case SCREEN32_ELEMENT_ID_B_MAIN_TASK:
            return select_section(ui, Section::Task, "Выбран раздел заданий");
        case SCREEN32_ELEMENT_ID_B_MAIN_PROFILE:
            return select_section(ui, Section::Profile, "Выбран раздел профилей");
        case SCREEN32_ELEMENT_ID_B_MAIN_NET:
            return select_section(ui, Section::Net, "Выбран сетевой раздел");
        case SCREEN32_ELEMENT_ID_B_MAIN_SERVICE:
            return select_section(ui, Section::Service, "Выбран сервисный раздел");
        case SCREEN32_ELEMENT_ID_B_MAIN_STATS:
            return select_section(ui, Section::Stats, "Выбран раздел статистики");
        case SCREEN32_ELEMENT_ID_B_MAIN_SUPPORT:
            return select_section(ui, Section::Support, "Выбран раздел помощи");
        default:
            return false;
    }
}

bool screen_page_main_menu::select_section(runtime& ui, Section section, const char* text) {
    _selectedSection = section;

    // В примере мы просто обновляем текст статуса.
    // На следующем шаге здесь можно будет вызывать Scene/State/Service
    // или выполнять переход на другую страницу через ui.show_page(...).
    return ui.text(SCREEN32_ELEMENT_ID_C_MAIN_MENU, text);
}

}  // namespace machine32::screen
