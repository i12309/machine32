#pragma once

#include <stdint.h>

#include "proto/machine.pb.h"
#include "runtime/SinglePageRuntime.h"

namespace machine32::screen {

// Системный фасад экрана: инициализация, транспорт и общие сервисные запросы.
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

    // Возвращает общий экземпляр фасада экрана.
    static Screen& getInstance();

    // Инициализирует доступные выходы экрана и runtime.
    bool init(uint8_t mode = static_cast<uint8_t>(Mode::Auto));
    // Обрабатывает транспорт экрана и события runtime.
    void tick();

    // Открывает указанную страницу, не зная её бизнес-смысла.
    template <typename T>
    bool showPage() {
        if (!_initialized || isSilent()) {
            return false;
        }
        return _runtime.start<T>();
    }

    // Возвращает предыдущую страницу, если runtime ее сохранил.
    bool back();

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
    bool hasDeviceInfo() const { return _hasDeviceInfo; }
    const char* screenUiVersion() const { return _deviceInfo.ui_version; }
    const char* screenFwVersion() const { return _deviceInfo.fw_version; }
    // Запрашивает у экрана служебную информацию об устройстве.
    bool updateScreenVersion();

private:
    static constexpr uint32_t kDeviceInfoRequestId = 0;

    Screen() = default;

    // Принимает события runtime и перенаправляет их в экземпляр класса.
    static void onRuntimeEvent(const Envelope& env, const screenlib::ScreenEventContext& ctx, void* userData);
    // Приводит внешний режим к поддерживаемому набору значений.
    static Mode sanitizeMode(uint8_t mode);
    // Сообщает, доступен ли физический экран.
    static bool supportsPhysical();
    // Сообщает, доступен ли web-экран.
    static bool supportsWeb();

    // Сбрасывает внутреннее состояние фасада.
    void reset();
    // Обрабатывает системные события runtime.
    void handleRuntimeEvent(const Envelope& env, const screenlib::ScreenEventContext& ctx);
    // Применяет полученную от экрана служебную информацию.
    void applyDeviceInfo(const DeviceInfo& info, const screenlib::ScreenEventContext& ctx);
    // Извлекает числовую версию из текстового значения.
    static int parseScreenVersion(const char* text);
    // Выбирает доступные выходы под запрошенный режим работы.
    bool selectOutputs(Mode requestedMode, bool& usePhysical, bool& useWeb);
    // Собирает конфигурацию runtime для выбранных выходов.
    void buildConfig(bool usePhysical, bool useWeb, screenlib::ScreenConfig& cfg) const;

    screenlib::SinglePageRuntime _runtime;
    Mode _mode = Mode::Silent;
    bool _initialized = false;
    bool _physicalEnabled = false;
    bool _webEnabled = false;
    bool _hasDeviceInfo = false;
    int _screenVersion = kUnknownScreenVersion;
    DeviceInfo _deviceInfo = DeviceInfo_init_zero;
    char _lastError[160] = {};
};

}  // namespace machine32::screen
