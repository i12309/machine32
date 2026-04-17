#include "Screen/runtime.h"

#include <string.h>

namespace machine32::screen {

namespace {

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

bool runtime::addPageController(screenlib::IPageController* controller) {
    if (_initialized || controller == nullptr || _customControllerCount >= kMaxCustomPageControllers) {
        return false;
    }

    if (screen32_find_page_descriptor(controller->pageId()) == nullptr) {
        return false;
    }

    for (size_t i = 0; i < _customControllerCount; ++i) {
        if (_customControllers[i] == controller || _customControllers[i]->pageId() == controller->pageId()) {
            return false;
        }
    }

    _customControllers[_customControllerCount++] = controller;
    return true;
}

bool runtime::init(const screenlib::ScreenConfig& config) {
    _snapshot = RuntimeSnapshot{};
    _registry = screenlib::PageRegistry{};

    if (!registerPages()) {
        _initialized = false;
        return false;
    }

    _screens.setPageRegistry(&_registry);
    _screens.setEventHandler(&runtime::onScreenEvent, this);

    _initialized = _screens.init(config);
    if (!_initialized) {
        return false;
    }

    return selectPage(SCREEN32_PAGE_ID_LOAD);
}

void runtime::tick() {
    if (!_initialized) {
        return;
    }

    _screens.tick();
}

bool runtime::selectPage(uint32_t pageId) {
    if (screen32_find_page_descriptor(pageId) == nullptr) {
        return false;
    }

    if (_screens.showPage(pageId)) {
        _snapshot.currentPageId = _registry.currentPage();
        return true;
    }

    if (_registry.setCurrentPage(pageId)) {
        _snapshot.currentPageId = pageId;
        return true;
    }

    return false;
}

bool runtime::dispatchEnvelopeForTest(const Envelope& env, const screenlib::ScreenEventContext& ctx) {
    if (!_initialized) {
        return false;
    }

    const bool handled = _registry.dispatchEnvelope(env, ctx);
    if (!handled) {
        captureEnvelopeCommon(env, ctx, false);
    }

    return handled;
}

const Screen32PageDescriptor* runtime::currentPageDescriptor() const {
    return screen32_find_page_descriptor(_snapshot.currentPageId);
}

void runtime::GeneratedPageController::bind(const Screen32PageDescriptor* descriptor, runtime* owner) {
    _descriptor = descriptor;
    _owner = owner;
}

uint32_t runtime::GeneratedPageController::pageId() const {
    return (_descriptor != nullptr) ? _descriptor->page_id : 0;
}

bool runtime::GeneratedPageController::onEnvelope(const Envelope& env, const screenlib::ScreenEventContext& ctx) {
    if (_descriptor == nullptr || _owner == nullptr) {
        return false;
    }

    _owner->capturePageEnvelope(_descriptor->page_id, env, ctx, true);
    return true;
}

void runtime::onScreenEvent(const Envelope& env, const screenlib::ScreenEventContext& ctx, void* userData) {
    runtime* self = static_cast<runtime*>(userData);
    if (self == nullptr) {
        return;
    }

    self->handleScreenEvent(env, ctx);
}

bool runtime::registerPages() {
    for (size_t i = 0; i < _customControllerCount; ++i) {
        if (!_registry.registerPage(_customControllers[i])) {
            return false;
        }
    }

    for (size_t i = 0; i < screen32_page_descriptor_count(); ++i) {
        const Screen32PageDescriptor* descriptor = &g_screen32_page_descriptors[i];
        if (hasCustomController(descriptor->page_id)) {
            continue;
        }

        _generatedControllers[i].bind(descriptor, this);
        if (!_registry.registerPage(&_generatedControllers[i])) {
            return false;
        }
    }

    return true;
}

bool runtime::hasCustomController(uint32_t pageId) const {
    for (size_t i = 0; i < _customControllerCount; ++i) {
        if (_customControllers[i] != nullptr && _customControllers[i]->pageId() == pageId) {
            return true;
        }
    }

    return false;
}

void runtime::handleScreenEvent(const Envelope& env, const screenlib::ScreenEventContext& ctx) {
    switch (env.which_payload) {
        case Envelope_current_page_tag:
            _snapshot.currentPageId = env.payload.current_page.page_id;
            captureEnvelopeCommon(env, ctx, false);
            break;
        case Envelope_device_info_tag:
        case Envelope_page_state_tag:
        case Envelope_element_state_tag:
        case Envelope_hello_tag:
        case Envelope_heartbeat_tag:
            captureEnvelopeCommon(env, ctx, false);
            break;
        case Envelope_button_event_tag:
        case Envelope_input_event_tag:
            break;
        default:
            captureEnvelopeCommon(env, ctx, false);
            break;
    }
}

void runtime::capturePageEnvelope(uint32_t pageId,
                                  const Envelope& env,
                                  const screenlib::ScreenEventContext& ctx,
                                  bool handledByPage) {
    _snapshot.currentPageId = pageId;
    captureEnvelopeCommon(env, ctx, handledByPage);
}

void runtime::captureEnvelopeCommon(const Envelope& env,
                                    const screenlib::ScreenEventContext& ctx,
                                    bool handledByPage) {
    _snapshot.lastEvent = CapturedScreenEvent{};
    _snapshot.lastEvent.payloadTag = env.which_payload;
    _snapshot.lastEvent.handledByPage = handledByPage;
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
                copy_text_safe(_snapshot.lastEvent.stringValue,
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
