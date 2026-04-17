#include "Screen/runtime.h"

#include <string.h>

#include "Screen/page.h"
#include "Screen/page_factory.h"

namespace machine32::screen {

namespace {

// Безопасное копирование строки в фиксированный буфер.
// Используется только для диагностического snapshot, чтобы не тащить туда
// String/std::string и не плодить лишние аллокации.
void copy_text_safe(char* dst, size_t dstSize, const char* src) {
    if (dst == nullptr || dstSize == 0) {
        return;
    }

    dst[0] = '\0';
    if (src == nullptr) {
        return;
    }

    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

}  // namespace

bool runtime::init(const screenlib::ScreenConfig& config) {
    _snapshot = RuntimeSnapshot{};
    _activePage.reset();

    _screens.setEventHandler(&runtime::on_screen_event, this);

    _initialized = _screens.init(config);
    if (!_initialized) {
        return false;
    }

    return activate_page(SCREEN32_PAGE_ID_LOAD, true);
}

void runtime::tick() {
    if (!_initialized) {
        return;
    }

    _screens.tick();
}

bool runtime::show_page(uint32_t page_id) {
    return activate_page(page_id, true);
}

bool runtime::text(uint32_t element_id, const char* text_value) {
    return _screens.setText(element_id, text_value);
}

bool runtime::value(uint32_t element_id, int32_t int_value) {
    return _screens.setValue(element_id, int_value);
}

bool runtime::visible(uint32_t element_id, bool is_visible) {
    return _screens.setVisible(element_id, is_visible);
}

const Screen32PageDescriptor* runtime::find_page(uint32_t page_id) const {
    return screen32_find_page_descriptor(page_id);
}

const Screen32ElementDescriptor* runtime::find_element(uint32_t element_id) const {
    return screen32_find_element_descriptor(element_id);
}

const Screen32PageDescriptor* runtime::current_page_descriptor() const {
    return find_page(_snapshot.currentPageId);
}

bool runtime::dispatch_envelope_for_test(const Envelope& env, const screenlib::ScreenEventContext& ctx) {
    if (!_initialized) {
        return false;
    }

    handle_screen_event(env, ctx);
    return true;
}

void runtime::on_screen_event(const Envelope& env, const screenlib::ScreenEventContext& ctx, void* userData) {
    runtime* self = static_cast<runtime*>(userData);
    if (self == nullptr) {
        return;
    }

    self->handle_screen_event(env, ctx);
}

void runtime::handle_screen_event(const Envelope& env, const screenlib::ScreenEventContext& ctx) {
    bool handled_by_page = false;

    switch (env.which_payload) {
        case Envelope_button_event_tag:
            handled_by_page = dispatch_button_event(env, ctx);
            break;
        case Envelope_input_event_tag:
            handled_by_page = dispatch_input_event(env, ctx);
            break;
        case Envelope_current_page_tag:
            sync_current_page_from_frontend(env.payload.current_page.page_id);
            break;
        default:
            break;
    }

    capture_event(env, ctx, handled_by_page);
}

bool runtime::activate_page(uint32_t page_id, bool send_show_command) {
    if (find_page(page_id) == nullptr) {
        return false;
    }

    if (_activePage != nullptr && _snapshot.currentPageId == page_id) {
        if (send_show_command) {
            _screens.showPage(page_id);
        }
        return true;
    }

    std::unique_ptr<page> next_page = create_page(page_id);
    if (!next_page) {
        return false;
    }

    if (send_show_command) {
        // Команда на frontend отправляется best-effort.
        // Даже если transport сейчас недоступен, backend всё равно может
        // локально переключить страницу и продолжать работать.
        _screens.showPage(page_id);
    }

    switch_active_page(std::move(next_page), page_id);
    return true;
}

void runtime::switch_active_page(std::unique_ptr<page> next_page, uint32_t page_id) {
    if (_activePage != nullptr) {
        _activePage->on_leave(*this);
    }

    _activePage = std::move(next_page);
    _snapshot.currentPageId = page_id;

    if (_activePage != nullptr) {
        _activePage->on_enter(*this);
    }
}

bool runtime::dispatch_button_event(const Envelope& env, const screenlib::ScreenEventContext& ctx) {
    (void)ctx;

    if (_activePage == nullptr) {
        return false;
    }

    const uint32_t event_page_id = env.payload.button_event.page_id;
    if (event_page_id != _snapshot.currentPageId) {
        return false;
    }

    return _activePage->on_button(*this, env.payload.button_event.element_id);
}

bool runtime::dispatch_input_event(const Envelope& env, const screenlib::ScreenEventContext& ctx) {
    (void)ctx;

    if (_activePage == nullptr) {
        return false;
    }

    const uint32_t event_page_id = env.payload.input_event.page_id;
    if (event_page_id != _snapshot.currentPageId) {
        return false;
    }

    if (env.payload.input_event.which_value == InputEvent_int_value_tag) {
        return _activePage->on_input_int(
            *this,
            env.payload.input_event.element_id,
            env.payload.input_event.value.int_value);
    }

    if (env.payload.input_event.which_value == InputEvent_string_value_tag) {
        return _activePage->on_input_text(
            *this,
            env.payload.input_event.element_id,
            env.payload.input_event.value.string_value);
    }

    return false;
}

void runtime::sync_current_page_from_frontend(uint32_t page_id) {
    // Этот путь нужен на случай reconnect / requestCurrentPage / внешнего
    // старта frontend, когда экран сам сообщает, какая страница сейчас активна.
    // Мы синхронизируем backend-состояние, но не отправляем showPage назад,
    // чтобы не устроить лишний круг команд.
    activate_page(page_id, false);
}

void runtime::capture_event(const Envelope& env, const screenlib::ScreenEventContext& ctx, bool handled_by_page) {
    _snapshot.lastEvent = CapturedScreenEvent{};
    _snapshot.lastEvent.payloadTag = env.which_payload;
    _snapshot.lastEvent.handledByPage = handled_by_page;
    _snapshot.lastEvent.context = ctx;

    switch (env.which_payload) {
        case Envelope_button_event_tag:
            _snapshot.lastEvent.pageId = env.payload.button_event.page_id;
            _snapshot.lastEvent.elementId = env.payload.button_event.element_id;
            break;
        case Envelope_input_event_tag:
            _snapshot.lastEvent.pageId = env.payload.input_event.page_id;
            _snapshot.lastEvent.elementId = env.payload.input_event.element_id;
            if (env.payload.input_event.which_value == InputEvent_int_value_tag) {
                _snapshot.lastEvent.hasIntValue = true;
                _snapshot.lastEvent.intValue = env.payload.input_event.value.int_value;
            } else if (env.payload.input_event.which_value == InputEvent_string_value_tag) {
                _snapshot.lastEvent.hasStringValue = true;
                copy_text_safe(
                    _snapshot.lastEvent.stringValue,
                    sizeof(_snapshot.lastEvent.stringValue),
                    env.payload.input_event.value.string_value);
            }
            break;
        case Envelope_current_page_tag:
            _snapshot.lastEvent.pageId = env.payload.current_page.page_id;
            break;
        case Envelope_page_state_tag:
            _snapshot.lastEvent.pageId = env.payload.page_state.page_id;
            break;
        case Envelope_element_state_tag:
            _snapshot.lastEvent.pageId = env.payload.element_state.page_id;
            if (env.payload.element_state.has_element) {
                _snapshot.lastEvent.elementId = env.payload.element_state.element.element_id;
            }
            break;
        default:
            break;
    }
}

}  // namespace machine32::screen
