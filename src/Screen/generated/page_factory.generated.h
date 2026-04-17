// АВТОСГЕНЕРИРОВАННЫЙ ФАЙЛ.
// Источник: ScreenUI/generated/shared/page_descriptors.generated.h
// НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ. Файл пересоздаётся pre-build скриптом machine32.

#pragma once

#include <memory>

#include "Screen/generic_page.h"
#include "Screen/page.h"
#include "generated/shared/page_descriptors.generated.h"

// Страница LOAD.
// Если проект содержит файл "Screen/Pages/load_page.h", будет создан
// специализированный page-класс.
// Иначе runtime автоматически использует универсальную заглушку.
#if __has_include("Screen/Pages/load_page.h")
#include "Screen/Pages/load_page.h"
#define MACHINE32_SCREEN_CREATE_LOAD() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::screen_page_load())
#else
#define MACHINE32_SCREEN_CREATE_LOAD() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::generic_page(screen32_find_page_descriptor(1u)))
#endif

// Страница MAIN_MENU.
// Если проект содержит файл "Screen/Pages/main_menu_page.h", будет создан
// специализированный page-класс.
// Иначе runtime автоматически использует универсальную заглушку.
#if __has_include("Screen/Pages/main_menu_page.h")
#include "Screen/Pages/main_menu_page.h"
#define MACHINE32_SCREEN_CREATE_MAIN_MENU() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::screen_page_main_menu())
#else
#define MACHINE32_SCREEN_CREATE_MAIN_MENU() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::generic_page(screen32_find_page_descriptor(2u)))
#endif

// Страница DEF_PAGE1.
// Если проект содержит файл "Screen/Pages/def_page1_page.h", будет создан
// специализированный page-класс.
// Иначе runtime автоматически использует универсальную заглушку.
#if __has_include("Screen/Pages/def_page1_page.h")
#include "Screen/Pages/def_page1_page.h"
#define MACHINE32_SCREEN_CREATE_DEF_PAGE1() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::screen_page_def_page1())
#else
#define MACHINE32_SCREEN_CREATE_DEF_PAGE1() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::generic_page(screen32_find_page_descriptor(3u)))
#endif

// Страница DEF_PAGE2.
// Если проект содержит файл "Screen/Pages/def_page2_page.h", будет создан
// специализированный page-класс.
// Иначе runtime автоматически использует универсальную заглушку.
#if __has_include("Screen/Pages/def_page2_page.h")
#include "Screen/Pages/def_page2_page.h"
#define MACHINE32_SCREEN_CREATE_DEF_PAGE2() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::screen_page_def_page2())
#else
#define MACHINE32_SCREEN_CREATE_DEF_PAGE2() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::generic_page(screen32_find_page_descriptor(4u)))
#endif

// Страница DEF_PAGE3.
// Если проект содержит файл "Screen/Pages/def_page3_page.h", будет создан
// специализированный page-класс.
// Иначе runtime автоматически использует универсальную заглушку.
#if __has_include("Screen/Pages/def_page3_page.h")
#include "Screen/Pages/def_page3_page.h"
#define MACHINE32_SCREEN_CREATE_DEF_PAGE3() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::screen_page_def_page3())
#else
#define MACHINE32_SCREEN_CREATE_DEF_PAGE3() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::generic_page(screen32_find_page_descriptor(5u)))
#endif

// Страница DEF_PAGE4.
// Если проект содержит файл "Screen/Pages/def_page4_page.h", будет создан
// специализированный page-класс.
// Иначе runtime автоматически использует универсальную заглушку.
#if __has_include("Screen/Pages/def_page4_page.h")
#include "Screen/Pages/def_page4_page.h"
#define MACHINE32_SCREEN_CREATE_DEF_PAGE4() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::screen_page_def_page4())
#else
#define MACHINE32_SCREEN_CREATE_DEF_PAGE4() \
    std::unique_ptr<machine32::screen::page>(new machine32::screen::generic_page(screen32_find_page_descriptor(6u)))
#endif

namespace machine32::screen::generated {

// Возвращает новый объект страницы по ID.
// Таблица создаётся автоматически на основе generated meta ScreenUI.
inline std::unique_ptr<page> create_page(uint32_t page_id) {
    switch (page_id) {
        case 1u: return MACHINE32_SCREEN_CREATE_LOAD();
        case 2u: return MACHINE32_SCREEN_CREATE_MAIN_MENU();
        case 3u: return MACHINE32_SCREEN_CREATE_DEF_PAGE1();
        case 4u: return MACHINE32_SCREEN_CREATE_DEF_PAGE2();
        case 5u: return MACHINE32_SCREEN_CREATE_DEF_PAGE3();
        case 6u: return MACHINE32_SCREEN_CREATE_DEF_PAGE4();
        default:
            return nullptr;
    }
}

static constexpr size_t kGeneratedPageCount = 6u;

}  // namespace machine32::screen::generated

#undef MACHINE32_SCREEN_CREATE_LOAD
#undef MACHINE32_SCREEN_CREATE_MAIN_MENU
#undef MACHINE32_SCREEN_CREATE_DEF_PAGE1
#undef MACHINE32_SCREEN_CREATE_DEF_PAGE2
#undef MACHINE32_SCREEN_CREATE_DEF_PAGE3
#undef MACHINE32_SCREEN_CREATE_DEF_PAGE4
