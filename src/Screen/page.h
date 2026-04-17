#pragma once

#include <stdint.h>

namespace machine32::screen {

class runtime;

// Базовый класс бизнес-логики одной backend-страницы.
//
// Важная идея новой схемы:
// - в памяти живёт только один активный объект страницы;
// - класс страницы знает только свой page_id и свои бизнес-события;
// - всё взаимодействие с экраном выполняется через фасад `runtime`.
class page {
public:
    virtual ~page() = default;

    // Возвращает page_id, с которым связан данный класс.
    // Этот ID приходит из ScreenUI/generated/shared и является источником истины
    // для маршрутизации событий между backend и frontend.
    virtual uint32_t page_id() const = 0;

    // Вызывается сразу после активации страницы.
    // Здесь удобно:
    // - заполнить текстовые поля;
    // - показать/скрыть элементы;
    // - инициировать запросы состояния у frontend, если это понадобится позже.
    virtual void on_enter(runtime& ui) { (void)ui; }

    // Вызывается перед уничтожением текущей страницы.
    // Здесь удобно:
    // - сохранить промежуточное состояние;
    // - завершить локальные операции страницы;
    // - очистить то, что не должно перетечь на следующую страницу.
    virtual void on_leave(runtime& ui) { (void)ui; }

    // Обработка нажатия на элемент текущей страницы.
    // Возвращает true, если событие обработано бизнес-логикой страницы.
    virtual bool on_button(runtime& ui, uint32_t element_id) {
        (void)ui;
        (void)element_id;
        return false;
    }

    // Обработка числового input-события.
    // Нужен для spinbox/slider/dropdown и других value-oriented элементов.
    virtual bool on_input_int(runtime& ui, uint32_t element_id, int32_t value) {
        (void)ui;
        (void)element_id;
        (void)value;
        return false;
    }

    // Обработка текстового input-события.
    // Нужен для textarea и других полей, которые присылают строку.
    virtual bool on_input_text(runtime& ui, uint32_t element_id, const char* value) {
        (void)ui;
        (void)element_id;
        (void)value;
        return false;
    }
};

}  // namespace machine32::screen
