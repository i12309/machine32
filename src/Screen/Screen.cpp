#include "Screen/Screen.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "Service/Log.h"
#include "Service/PinList.h"

namespace machine32::screen {

namespace {

const char* payloadName(pb_size_t tag) {
    switch (tag) {
        case Envelope_show_page_tag: return "show_page";
        case Envelope_set_text_tag: return "set_text";
        case Envelope_set_color_tag: return "set_color";
        case Envelope_set_visible_tag: return "set_visible";
        case Envelope_set_value_tag: return "set_value";
        case Envelope_set_batch_tag: return "set_batch";
        case Envelope_button_event_tag: return "button_event";
        case Envelope_input_event_tag: return "input_event";
        case Envelope_heartbeat_tag: return "heartbeat";
        case Envelope_hello_tag: return "hello";
        case Envelope_request_device_info_tag: return "request_device_info";
        case Envelope_device_info_tag: return "device_info";
        case Envelope_request_current_page_tag: return "request_current_page";
        case Envelope_current_page_tag: return "current_page";
        case Envelope_request_page_state_tag: return "request_page_state";
        case Envelope_page_state_tag: return "page_state";
        case Envelope_request_element_state_tag: return "request_element_state";
        case Envelope_element_state_tag: return "element_state";
        default: return "unknown";
    }
}

const char* endpointName(const screenlib::ScreenEventContext& ctx) {
    if (ctx.isPhysical) return "physical";
    if (ctx.isWeb) return "web";
    return "unknown";
}

}  // namespace

// Возвращает общий экземпляр фасада экрана.
Screen& Screen::getInstance() {
    static Screen instance;
    return instance;
}

// Инициализирует runtime и настраивает доступные каналы вывода.
bool Screen::init(uint8_t modeValue) {
    reset();

    Mode requestedMode = sanitizeMode(modeValue);
    bool usePhysical = false;
    bool useWeb = false;

    if (!selectOutputs(requestedMode, usePhysical, useWeb)) {
        return false;
    }

    if (requestedMode == Mode::Auto && !usePhysical && !useWeb) {
        requestedMode = Mode::Silent;
    }

    _mode = requestedMode;
    _physicalEnabled = usePhysical;
    _webEnabled = useWeb;

    if (_mode == Mode::Silent) {
        _initialized = true;
        snprintf(_lastError, sizeof(_lastError), "%s", "");
        return true;
    }

    screenlib::ScreenConfig cfg{};
    buildConfig(usePhysical, useWeb, cfg);
    if (cfg.physical.enabled && cfg.physical.type == screenlib::OutputType::Uart) {
        Log::D("[Screen] physical uart config: baud=%lu rx=%d tx=%d",
               static_cast<unsigned long>(cfg.physical.uart.baud),
               static_cast<int>(cfg.physical.uart.rxPin),
               static_cast<int>(cfg.physical.uart.txPin));
    }
    _runtime.setEventObserver(&Screen::onRuntimeEvent, this);

    if (!_runtime.init(cfg)) {
        snprintf(_lastError, sizeof(_lastError), "screen init failed: %s", _runtime.lastError());
        Log::E("[Screen] %s", _lastError);
        return false;
    }

    _initialized = true;
    snprintf(_lastError, sizeof(_lastError), "%s", "");

    Log::D("[Screen] init ok, mode=%u, physical=%d, web=%d",
           static_cast<unsigned>(_mode),
           _physicalEnabled ? 1 : 0,
           _webEnabled ? 1 : 0);
    updateScreenVersion();
    return true;
}

// Обрабатывает события runtime, если экран активен.
void Screen::tick() {
    if (!_initialized || isSilent()) {
        return;
    }
    _runtime.tick();

    const bool physical = _physicalEnabled && _runtime.connectedPhysical();
    const bool web = _webEnabled && _runtime.connectedWeb();
    if (!_connectionStateInitialized ||
        _lastPhysicalConnected != physical ||
        _lastWebConnected != web) {
        Log::D("[Screen] link state changed: physical=%d web=%d",
               physical ? 1 : 0,
               web ? 1 : 0);
        _lastPhysicalConnected = physical;
        _lastWebConnected = web;
        _connectionStateInitialized = true;
    }
}

// Возвращает runtime на предыдущую страницу без знания бизнес-смысла перехода.
bool Screen::back() {
    if (!_initialized || isSilent()) {
        return false;
    }
    return _runtime.back();
}

// Возвращает состояние физического соединения экрана.
bool Screen::connectedPhysical() const {
    return _initialized && _physicalEnabled && _runtime.connectedPhysical();
}

// Возвращает состояние web-соединения экрана.
bool Screen::connectedWeb() const {
    return _initialized && _webEnabled && _runtime.connectedWeb();
}

// Запрашивает у экрана текущую служебную информацию об устройстве.
bool Screen::updateScreenVersion() {
    if (!_initialized || isSilent()) {
        _screenVersion = kUnknownScreenVersion;
        return false;
    }
    const bool requested = _runtime.screens().requestDeviceInfo(kDeviceInfoRequestId);
    Log::D("[Screen] tx request_device_info request_id=%lu status=%s",
           static_cast<unsigned long>(kDeviceInfoRequestId),
           requested ? "ok" : "fail");
    return requested;
}

// Перенаправляет событие runtime в экземпляр фасада.
void Screen::onRuntimeEvent(const Envelope& env, const screenlib::ScreenEventContext& ctx, void* userData) {
    Screen* self = static_cast<Screen*>(userData);
    if (self != nullptr) {
        self->handleRuntimeEvent(env, ctx);
    }
}

// Разбирает только системные события runtime.
void Screen::handleRuntimeEvent(const Envelope& env, const screenlib::ScreenEventContext& ctx) {
    Log::D("[Screen] rx payload=%s(%u) from %s endpoint=%u",
           payloadName(env.which_payload),
           static_cast<unsigned>(env.which_payload),
           endpointName(ctx),
           static_cast<unsigned>(ctx.endpointId));

    switch (env.which_payload) {
        case Envelope_hello_tag:
            if (env.payload.hello.has_device_info) {
                applyDeviceInfo(env.payload.hello.device_info, ctx);
            }
            break;

        case Envelope_device_info_tag:
            applyDeviceInfo(env.payload.device_info, ctx);
            break;

        default:
            break;
    }
}

// Сохраняет служебную информацию, пришедшую от экрана.
void Screen::applyDeviceInfo(const DeviceInfo& info, const screenlib::ScreenEventContext& ctx) {
    _deviceInfo = info;
    _hasDeviceInfo = true;
    _screenVersion = parseScreenVersion(info.ui_version);

    Log::D("[Screen] device info from %s: fw='%s' ui='%s' type='%s' client='%s'",
           endpointName(ctx),
           info.fw_version,
           info.ui_version,
           info.screen_type,
           info.client_type);
}

// Извлекает номер версии из текста вида "UI: 123".
int Screen::parseScreenVersion(const char* text) {
    if (text == nullptr || *text == '\0') {
        return kUnknownScreenVersion;
    }

    while (*text != '\0' && !isdigit(static_cast<unsigned char>(*text))) {
        ++text;
    }
    if (*text == '\0') {
        return kUnknownScreenVersion;
    }
    return atoi(text);
}

// Нормализует внешний режим работы экрана.
Screen::Mode Screen::sanitizeMode(uint8_t mode) {
    switch (mode) {
        case static_cast<uint8_t>(Mode::Auto): return Mode::Auto;
        case static_cast<uint8_t>(Mode::Silent): return Mode::Silent;
        case static_cast<uint8_t>(Mode::PhysicalOnly): return Mode::PhysicalOnly;
        case static_cast<uint8_t>(Mode::WebOnly): return Mode::WebOnly;
        case static_cast<uint8_t>(Mode::Both): return Mode::Both;
        default: return Mode::Auto;
    }
}

// В текущей конфигурации физический экран всегда поддерживается.
bool Screen::supportsPhysical() {
    return true;
}

// В текущей конфигурации web-экран всегда поддерживается.
bool Screen::supportsWeb() {
    return true;
}

// Возвращает фасад в исходное состояние.
void Screen::reset() {
    _mode = Mode::Silent;
    _initialized = false;
    _physicalEnabled = false;
    _webEnabled = false;
    _connectionStateInitialized = false;
    _lastPhysicalConnected = false;
    _lastWebConnected = false;
    _hasDeviceInfo = false;
    _screenVersion = kUnknownScreenVersion;
    _deviceInfo = DeviceInfo{};
    _runtime = screenlib::SinglePageRuntime();
    _lastError[0] = '\0';
}

// Выбирает активные выходы под заданный режим работы.
bool Screen::selectOutputs(Mode requestedMode, bool& usePhysical, bool& useWeb) {
    usePhysical = false;
    useWeb = false;

    const bool physicalAvailable = supportsPhysical();
    const bool webAvailable = supportsWeb();

    switch (requestedMode) {
        case Mode::Auto:
            usePhysical = physicalAvailable;
            useWeb = webAvailable;
            return true;

        case Mode::Silent:
            return true;

        case Mode::PhysicalOnly:
            if (!physicalAvailable) {
                snprintf(_lastError, sizeof(_lastError), "physical screen is not available");
                return false;
            }
            usePhysical = true;
            return true;

        case Mode::WebOnly:
            if (!webAvailable) {
                snprintf(_lastError, sizeof(_lastError), "web screen is not available");
                return false;
            }
            useWeb = true;
            return true;

        case Mode::Both:
            if (!physicalAvailable || !webAvailable) {
                if (!physicalAvailable && !webAvailable) {
                    snprintf(_lastError, sizeof(_lastError), "physical and web screens are not available");
                } else if (!physicalAvailable) {
                    snprintf(_lastError, sizeof(_lastError), "physical screen is not available");
                } else {
                    snprintf(_lastError, sizeof(_lastError), "web screen is not available");
                }
                return false;
            }
            usePhysical = true;
            useWeb = true;
            return true;
    }

    snprintf(_lastError, sizeof(_lastError), "unsupported screen mode");
    return false;
}

// Собирает конфигурацию runtime для выбранного набора выходов.
void Screen::buildConfig(bool usePhysical, bool useWeb, screenlib::ScreenConfig& cfg) const {
    cfg = screenlib::ScreenConfig{};

    if (usePhysical) {
        cfg.physical.enabled = true;
        cfg.physical.type = screenlib::OutputType::Uart;
        cfg.physical.uart.rxPin = TFT_RX_PIN;
        cfg.physical.uart.txPin = TFT_TX_PIN;
    }

    if (useWeb) {
        cfg.web.enabled = true;
        cfg.web.type = screenlib::OutputType::WsServer;
        cfg.web.wsServer.port = kDefaultWebPort;
    }

    if (usePhysical && useWeb) {
        cfg.mirrorMode = screenlib::MirrorMode::Both;
    } else if (useWeb) {
        cfg.mirrorMode = screenlib::MirrorMode::WebOnly;
    } else {
        cfg.mirrorMode = screenlib::MirrorMode::PhysicalOnly;
    }
}

}  // namespace machine32::screen
