#pragma once

#include <stdint.h>

#include "Screen/page.h"
#include "generated/shared/element_ids.generated.h"
#include "generated/shared/page_ids.generated.h"

namespace machine32::screen {

// Бизнес-логика страницы главного меню.
//
// В page-классе остаётся только то, что действительно относится к сценарию:
// - какой page_id он обслуживает;
// - что делать при входе;
// - что делать при нажатии на конкретные элементы.
//
// Всё низкоуровневое общение с экраном скрыто в runtime.
class screen_page_main_menu final : public page {
public:
    // ID страницы задаётся один раз и дальше используется всей бизнес-логикой.
    uint32_t page_id() const override { return SCREEN32_PAGE_ID_MAIN_MENU; }

    // При входе заполняем статусную строку экрана.
    void on_enter(runtime& ui) override;

    // Обрабатываем только нажатия, относящиеся к главному меню.
    bool on_button(runtime& ui, uint32_t element_id) override;

private:
    enum class Section : uint32_t {
        None = 0,
        Task,
        Profile,
        Net,
        Service,
        Stats,
        Support
    };

    // Централизованная обработка выбора раздела.
    bool select_section(runtime& ui, Section section, const char* text);

    Section _selectedSection = Section::None;
};

}  // namespace machine32::screen
