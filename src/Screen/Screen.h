#pragma once

#include <stdint.h>

#include "runtime/SinglePageRuntime.h"

namespace machine32::screen {

class Screen {
public:
    enum class Mode : uint8_t {
        Auto = 0,
        Silent = 1,
        PhysicalOnly = 2,
        WebOnly = 3,
        Both = 4,
    };

    static constexpr uint16_t kDefaultWebPort = 81;
    static constexpr int kUnknownScreenVersion = -1;

    static Screen& getInstance();

    bool init(uint8_t mode = static_cast<uint8_t>(Mode::Auto));
    void tick();

    template <typename T>
    bool showPage() {
        if (!_initialized || isSilent()) {
            return false;
        }
        return _runtime.start<T>();
    }

    bool showLoad();
    bool showMain();

    bool isInitialized() const { return _initialized; }
    bool isSilent() const { return _mode == Mode::Silent; }
    bool anyOutputEnabled() const { return _physicalEnabled || _webEnabled; }
    bool hasPhysical() const { return _physicalEnabled; }
    bool hasWeb() const { return _webEnabled; }
    bool connectedPhysical() const;
    bool connectedWeb() const;

    Mode mode() const { return _mode; }
    const char* lastError() const { return _lastError; }

    int getScreenVersion() const { return _screenVersion; }
    bool updateScreenVersion();

private:
    Screen() = default;

    static Mode sanitizeMode(uint8_t mode);
    static bool supportsPhysical();
    static bool supportsWeb();

    void reset();
    bool selectOutputs(Mode requestedMode, bool& usePhysical, bool& useWeb);
    void buildConfig(bool usePhysical, bool useWeb, screenlib::ScreenConfig& cfg) const;

    screenlib::SinglePageRuntime _runtime;
    Mode _mode = Mode::Silent;
    bool _initialized = false;
    bool _physicalEnabled = false;
    bool _webEnabled = false;
    int _screenVersion = kUnknownScreenVersion;
    char _lastError[160] = {};
};

}  // namespace machine32::screen
