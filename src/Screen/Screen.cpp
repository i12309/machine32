#include "Screen/Screen.h"

#include <stdio.h>
#include <string.h>

#include "Screen/load_page.h"
#include "Service/Log.h"
#include "Service/PinList.h"
#include "Screen/main.h"

namespace machine32::screen {

Screen& Screen::getInstance() {
    static Screen instance;
    return instance;
}

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
    return true;
}

void Screen::tick() {
    if (!_initialized || isSilent()) {
        return;
    }
    _runtime.tick();
}

bool Screen::showLoad() {
    if (!_initialized) {
        snprintf(_lastError, sizeof(_lastError), "screen is not initialized");
        return false;
    }
    if (isSilent()) {
        return true;
    }
    return showPage<LoadPage>();
}

bool Screen::showMain() {
    if (!_initialized) {
        snprintf(_lastError, sizeof(_lastError), "screen is not initialized");
        return false;
    }
    if (isSilent()) {
        return true;
    }
    return showPage<Main>();
}

bool Screen::connectedPhysical() const {
    return _initialized && _physicalEnabled && _runtime.connectedPhysical();
}

bool Screen::connectedWeb() const {
    return _initialized && _webEnabled && _runtime.connectedWeb();
}

bool Screen::updateScreenVersion() {
    _screenVersion = kUnknownScreenVersion;
    return false;
}

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

bool Screen::supportsPhysical() {
    return true;
}

bool Screen::supportsWeb() {
    return true;
}

void Screen::reset() {
    _mode = Mode::Silent;
    _initialized = false;
    _physicalEnabled = false;
    _webEnabled = false;
    _screenVersion = kUnknownScreenVersion;
    _runtime = screenlib::SinglePageRuntime();
    _lastError[0] = '\0';
}

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
