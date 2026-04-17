#pragma once

#include <memory>
#include <stdint.h>

#include "Screen/page.h"
#include "generated/shared/element_descriptors.generated.h"
#include "generated/shared/page_descriptors.generated.h"
#include "system/ScreenSystem.h"

namespace machine32::screen {

// Последнее полученное от экрана событие.
//
// Это отладочный и диагностический снимок. Он не участвует в маршрутизации,
// но помогает быстро понять, что именно пришло со стороны screen32.
struct CapturedScreenEvent {
    pb_size_t payloadTag = 0;
    uint32_t pageId = 0;
    uint32_t elementId = 0;
    bool handledByPage = false;
    bool hasIntValue = false;
    bool hasStringValue = false;
    int32_t intValue = 0;
    char stringValue[32] = {};
    screenlib::ScreenEventContext context{};
};

// Краткий runtime-снимок backend-стороны screen32.
struct RuntimeSnapshot {
    uint32_t currentPageId = 0;
    CapturedScreenEvent lastEvent{};
};

// Главный фасад работы backend с экраном.
//
// Цели класса:
// - скрыть детали ScreenSystem и протокольного routing;
// - держать только одну активную бизнес-страницу;
// - дать страницам простой API для работы по element_id/page_id;
// - упростить добавление новой логики до сценария "написал page-класс и всё".
class runtime {
public:
    // Инициализирует screen runtime и поднимает первый экран.
    bool init(const screenlib::ScreenConfig& config);

    // Главный runtime tick. Его должен вызывать внешний цикл приложения.
    void tick();

    // Показать страницу по ID.
    //
    // Метод:
    // 1. Создаёт новый page-объект через фабрику.
    // 2. Завершает текущую страницу.
    // 3. Делает новую страницу активной.
    // 4. Пытается отправить frontend команду showPage(page_id).
    //
    // Важно: локальное переключение считается успешным даже если frontend в
    // данный момент не подключён. Это позволяет backend-логике жить своей
    // жизнью и не зависеть жёстко от наличия transport.
    bool show_page(uint32_t page_id);

    // Установить текст элемента по ID.
    bool text(uint32_t element_id, const char* text);

    // Установить числовое значение элемента по ID.
    bool value(uint32_t element_id, int32_t value);

    // Показать или скрыть элемент по ID.
    bool visible(uint32_t element_id, bool visible);

    // Найти descriptor страницы по ID.
    const Screen32PageDescriptor* find_page(uint32_t page_id) const;

    // Найти descriptor элемента по ID.
    const Screen32ElementDescriptor* find_element(uint32_t element_id) const;

    // Возвращает descriptor активной страницы.
    const Screen32PageDescriptor* current_page_descriptor() const;

    // Текущий active page_id на стороне backend.
    uint32_t current_page_id() const { return _snapshot.currentPageId; }

    // Отладочный снимок runtime.
    const RuntimeSnapshot& snapshot() const { return _snapshot; }

    // Последняя ошибка ScreenSystem, если init/bootstrap не удались.
    const char* last_error() const { return _screens.lastError(); }

    // Низкоуровневый выход к ScreenSystem.
    //
    // Основная логика должна предпочитать методы фасада выше. Но этот метод
    // оставлен как аварийный выход для редких случаев, когда понадобятся
    // дополнительные команды screenLIB без раздувания public API runtime.
    screenlib::ScreenSystem& system() { return _screens; }
    const screenlib::ScreenSystem& system() const { return _screens; }

    // Тестовый helper для локального прогона входящих событий без transport.
    bool dispatch_envelope_for_test(const Envelope& env, const screenlib::ScreenEventContext& ctx = {});

private:
    // Единая точка приёма входящих сообщений от screenLIB.
    static void on_screen_event(const Envelope& env, const screenlib::ScreenEventContext& ctx, void* userData);

    // Внутренняя обработка входящего сообщения.
    void handle_screen_event(const Envelope& env, const screenlib::ScreenEventContext& ctx);

    // Активирует страницу локально и при необходимости отправляет showPage на frontend.
    bool activate_page(uint32_t page_id, bool send_show_command);

    // Переключает page-object и корректно вызывает on_leave/on_enter.
    void switch_active_page(std::unique_ptr<page> next_page, uint32_t page_id);

    // Обработчики конкретных типов событий.
    bool dispatch_button_event(const Envelope& env, const screenlib::ScreenEventContext& ctx);
    bool dispatch_input_event(const Envelope& env, const screenlib::ScreenEventContext& ctx);

    // Отдельный путь синхронизации, когда frontend сам сообщил текущую страницу.
    void sync_current_page_from_frontend(uint32_t page_id);

    // Заполняет отладочный снимок события.
    void capture_event(const Envelope& env, const screenlib::ScreenEventContext& ctx, bool handled_by_page);

    screenlib::ScreenSystem _screens;
    std::unique_ptr<page> _activePage;
    RuntimeSnapshot _snapshot{};
    bool _initialized = false;
};

}  // namespace machine32::screen
