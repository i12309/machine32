#include "Screen/Screen.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "Service/Log.h"
#include "Service/PinList.h"
#include "config/ScreenConfig.h"
#include "link/UartLink.h"
#include "link/WebSocketServerLink.h"

namespace machine32::screen {

Screen::Screen()
    : _runtime(new screenlib::PageRuntime()) {}

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
    if (_runtime == nullptr) {
        _runtime.reset(new screenlib::PageRuntime());
    }

    if (!_runtime->init(cfg)) {
        snprintf(_lastError, sizeof(_lastError), "screen init failed");
        Log::E("[Screen] %s", _lastError);
        return false;
    }

    if (usePhysical && !bootstrapPhysicalBridge(cfg)) {
        snprintf(_lastError, sizeof(_lastError), "physical bridge bootstrap failed");
        Log::E("[Screen] %s", _lastError);
        return false;
    }

    if (useWeb && !bootstrapWebBridge(cfg)) {
        snprintf(_lastError, sizeof(_lastError), "web bridge bootstrap failed");
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
    if (_runtime == nullptr) {
        return;
    }
    _runtime->tick();

    const bool physical = _physicalEnabled && _physicalBridge != nullptr && _physicalBridge->connected();
    const bool web = _webEnabled && _webBridge != nullptr && _webBridge->connected();
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
    return _runtime != nullptr && _runtime->back();
}

// Возвращает состояние физического соединения экрана.
bool Screen::connectedPhysical() const {
    return _initialized && _physicalEnabled && _physicalBridge != nullptr && _physicalBridge->connected();
}

// Возвращает состояние web-соединения экрана.
bool Screen::connectedWeb() const {
    return _initialized && _webEnabled && _webBridge != nullptr && _webBridge->connected();
}

// Запрашивает у экрана текущую служебную информацию об устройстве.
bool Screen::updateScreenVersion() {
    // TODO object-model: вернуть запрос device_info через OOB handler PageRuntime.
    _screenVersion = kUnknownScreenVersion;
    Log::D("[Screen] updateScreenVersion is disabled during object-model migration");
    return false;
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
    _screenVersion = kUnknownScreenVersion;
    _deviceInfo = DeviceInfo{};
    _runtime.reset(new screenlib::PageRuntime());
    _physicalBridge.reset();
    _webBridge.reset();
    _physicalTransport.reset();
    _webTransport.reset();
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

bool Screen::bootstrapPhysicalBridge(const screenlib::ScreenConfig& cfg) {
    if (!cfg.physical.enabled) {
        return false;
    }

    switch (cfg.physical.type) {
        case screenlib::OutputType::Uart: {
            std::unique_ptr<UartLink> uart(new UartLink(Serial1));
            UartLink::Config uartCfg{};
            uartCfg.baud = cfg.physical.uart.baud;
            uartCfg.rxPin = cfg.physical.uart.rxPin;
            uartCfg.txPin = cfg.physical.uart.txPin;
            uart->begin(uartCfg);
            _physicalTransport = std::move(uart);
            break;
        }

        case screenlib::OutputType::WsServer: {
            std::unique_ptr<WebSocketServerLink> ws(
                new WebSocketServerLink(cfg.physical.wsServer.port));
            ws->begin();
            _physicalTransport = std::move(ws);
            break;
        }

        default:
            return false;
    }

    _physicalBridge.reset(new ScreenBridge(*_physicalTransport));
    _runtime->attachPhysicalBridge(_physicalBridge.get());
    return true;
}

bool Screen::bootstrapWebBridge(const screenlib::ScreenConfig& cfg) {
    if (!cfg.web.enabled) {
        return false;
    }

    switch (cfg.web.type) {
        case screenlib::OutputType::Uart: {
            std::unique_ptr<UartLink> uart(new UartLink(Serial2));
            UartLink::Config uartCfg{};
            uartCfg.baud = cfg.web.uart.baud;
            uartCfg.rxPin = cfg.web.uart.rxPin;
            uartCfg.txPin = cfg.web.uart.txPin;
            uart->begin(uartCfg);
            _webTransport = std::move(uart);
            break;
        }

        case screenlib::OutputType::WsServer: {
            std::unique_ptr<WebSocketServerLink> ws(
                new WebSocketServerLink(cfg.web.wsServer.port));
            ws->begin();
            _webTransport = std::move(ws);
            break;
        }

        default:
            return false;
    }

    _webBridge.reset(new ScreenBridge(*_webTransport));
    _runtime->attachWebBridge(_webBridge.get());
    return true;
}

}  // namespace machine32::screen
