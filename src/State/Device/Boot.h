#pragma once
#include "Core.h"
#include "Catalog.h"
#include "Machine/Machine.h"
#include "Service/HServer.h"
#include "Service/NVS.h"
#include "Service/ESPUpdate.h"
#include "Service/NexUpdate.h"
#include "Service/Licence.h"
#include "Service/MQTTC.h"
#include "Service/Stats.h"
#include "Service/WiFiConfig.h"
#include "Screen/Screen.h"
#include "Screen/page/main/info.h"
#include "Screen/page/main/INIT.h"
#include "Screen/page/main/load.h"
#include "Screen/page/main/main.h"
#include "version.h"
#include "State/State.h"
#include "Controller/McpTrigger.h"
#include "App/App.h"

using machine32::screen::Info;
using machine32::screen::Init;
using machine32::screen::Load;
using machine32::screen::Main;
using machine32::screen::Screen;

// Состояние начальной загрузки устройства.
class Boot : public State {
private:
    // Контекст загрузки, который живет между шагами boot-плана.
    struct BootContext {
        bool wifiConnected = false;
        bool screenReady = false;
        bool waitingInfo = false;
        bool abortRequested = false;
        bool haltRequested = false;
        State::Type abortState = State::Type::NULL_STATE;
        uint32_t screenWaitStartedAt = 0;
        uint32_t screenLastProbeAt = 0;
        bool loadShown = false;
    };

    static constexpr uint32_t kScreenReadyTimeoutMs = 8000;
    static constexpr uint32_t kScreenProbeIntervalMs = 400;

    // Возвращает общий контекст текущей загрузки.
    static BootContext& context() {
        static BootContext ctx;
        return ctx;
    }

    // Сбрасывает контекст перед новым запуском BOOT.
    static void resetContext() {
        context() = BootContext();
    }

    // Запрашивает аварийный выход из BOOT в указанное состояние.
    static void requestAbort(State::Type state) {
        BootContext& ctx = context();
        ctx.abortRequested = true;
        ctx.abortState = state;
    }

    // Запрашивает остановку BOOT без перехода в другое состояние.
    static void requestHalt() {
        context().haltRequested = true;
    }

    // Пишет служебный статус загрузки в лог.
    static void setStatus(const String& text) {
        Log::D("BOOT: %s", text.c_str());
    }

    // Пишет ошибочный статус загрузки в лог.
    static void setStatusFail(const String& text) {
        Log::E("BOOT: %s", text.c_str());
    }
public:
    // Создает состояние BOOT.
    Boot() : State(State::Type::BOOT) {}

    // Формирует пошаговый план загрузки устройства.
    void oneRun() override {
        State::oneRun();
        resetContext();

        PlanManager& plan = App::plan();
        plan.beginPlan(this->type());

        plan.addAction(State::Type::ACTION, &Boot::LogStart, "LogStart");
        plan.addAction(State::Type::ACTION, &Boot::InitScreen, "InitScreen");
        plan.addAction(State::Type::ACTION, &Boot::ShowLoad, "ShowLoad");
        plan.addAction(State::Type::ACTION, &Boot::InitFileSystem, "InitFileSystem");
        plan.addAction(State::Type::ACTION, &Boot::InitNVS, "InitNVS");
        plan.addAction(State::Type::ACTION, &Boot::LoadConfig, "LoadConfig");
        plan.addAction(State::Type::ACTION, &Boot::ConnectWiFi, "ConnectWiFi");
        plan.addAction(State::Type::ACTION, &Boot::UpdateESP, "UpdateESP");
        plan.addAction(State::Type::ACTION, &Boot::SetScreenVersion, "SetScreenVersion");
        plan.addAction(State::Type::ACTION, &Boot::StartHttp, "StartHTTP");
        plan.addAction(State::Type::ACTION, &Boot::ConnectMQTT, "ConnectMQTT");
        plan.addAction(State::Type::ACTION, &Boot::LoadData, "LoadData");
        plan.addAction(State::Type::ACTION, &Boot::InitRegistry, "InitRegistry");
        plan.addAction(State::Type::ACTION, &Boot::RegisterTriggers, "RegisterTriggers");

    }

    // Выполняет boot-план и удерживает BOOT, пока открыт новый экран Info.
    State* run() override {
        PlanManager& plan = App::plan();
        BootContext& boot = context();

        if (boot.haltRequested) {
            plan.clear();
            return this;
        }

        if (boot.abortRequested) {
            plan.clear();
            return Factory(boot.abortState);
        }

        if (!plan.hasPending()) {
            Log::L(" === END LOAD v.%s", Version::makeDeviceVersion(NexUpdate::getInstance().getCurrentVersion()).c_str());
            Data::param.reset();
            App::ctx().reg.reset();
            App::mode() = Mode::NORMAL;
            App::diag().clearErrors();
            if (*App::cfg().checkSystem == 1) {
                setStatus("Проверка устройства");
                return Factory(State::Type::CHECK);
            }
            ESPUpdate::getInstance().markCurrentFirmwareValid(); // Если не CHECK_SYSTEM то считаем что загрузка прошла успешно и валидируем новую прошивку, что бы при следующей загрузки не было проверки и возможного отката.
            return Factory(State::Type::IDLE);
        }

        return Factory(plan.nextType(this->type()));
    }

private:
    // Пишет стартовую служебную информацию о контроллере.
    static bool LogStart() {
        Log::L(" === START v.%s", Version::makeDeviceVersion(NexUpdate::getInstance().getCurrentVersion()).c_str());
        Log::D("ESP32 Chip: %s Rev %d", ESP.getChipModel(), ESP.getChipRevision());
        Log::D("Flash size: %d", ESP.getFlashChipSize());
        return true;
    }

    // Инициализирует новый экранный runtime.
    static bool InitScreen() {
        BootContext& boot = context();
        boot.screenReady = false;
        boot.loadShown = false;
        boot.screenWaitStartedAt = millis();
        boot.screenLastProbeAt = 0;

        Screen& screen = Screen::getInstance();
        if (!screen.init(static_cast<uint8_t>(Screen::Mode::PhysicalOnly))) {
            Log::E(" === ERROR Screen Init: %s", screen.lastError());
            requestAbort(State::Type::NULL_STATE);
            return true;
        }

        boot.screenReady = !screen.isSilent();
        return true;
    }

    // Показывает экран загрузки через новый класс страницы.
    static bool ShowLoad() {
        BootContext& boot = context();
        if (!boot.screenReady || boot.loadShown) return true;

        Screen& screen = Screen::getInstance();
        if (!screen.hasDeviceInfo()) {
            const uint32_t now = millis();
            if (boot.screenWaitStartedAt == 0) boot.screenWaitStartedAt = now;

            if (now - boot.screenLastProbeAt >= kScreenProbeIntervalMs) {
                screen.updateScreenVersion();
                boot.screenLastProbeAt = now;
            }

            if (now - boot.screenWaitStartedAt < kScreenReadyTimeoutMs) {
                App::plan().resetByActionName("ShowLoad");
                return true;
            }

            Log::E("[BOOT] Screen device info timeout after %lu ms, continue without handshake.",
                   static_cast<unsigned long>(kScreenReadyTimeoutMs));
        }

        setStatus("Загрузка интерфейса");
        if (!Load::show()) {
            Log::E(" === ERROR ShowLoad: %s", Screen::getInstance().lastError());
            requestAbort(State::Type::NULL_STATE);
        } else {
            boot.loadShown = true;
        }
        return true;
    }

    // Инициализирует файловую систему устройства.
    static bool InitFileSystem() {
        setStatus("Инициализация файловой системы");
        if (FileSystem::init(true)) {
            return true;
        }

        setStatusFail("Ошибка файловой системы");
        Log::E(" === ERROR FileSystem Init ");
        requestAbort(State::Type::NULL_STATE);
        return true;
    }

    // Инициализирует NVS и обрабатывает счетчик неудачных загрузок.
    static bool InitNVS() {
        setStatus("Инициализация NVS");
        NVS& nvs = NVS::getInstance();
        nvs.init();

        int bootCount = nvs.getInt("boot_count", 0, "boot");
        bootCount++;
        if (bootCount > 3) {
            if (nvs.getInt("ota_pending", 0, "boot") == 1) {
                setStatusFail("OTA boot failed, rollback");
                if (ESPUpdate::getInstance().rollbackToPreviousPartition("boot_count exceeded")) {
                    requestHalt();
                    return true;
                }
            }
            nvs.setInt("boot_count", 0, "boot");
            nvs.setInt("ota_pending", 0, "boot");
            Info::showInfo("", "Что-то пошло не так!", "Проверьте параметры", []() { Init::show(); });
            requestAbort(State::Type::NULL_STATE);
            return true;
        }
        nvs.setInt("boot_count", bootCount, "boot");
        return true;
    }

    // Загружает конфигурацию и выбирает тип машины.
    static bool LoadConfig() {
        setStatus("Загрузка конфигурации");
        if (Core::config.load(!context().screenReady)) {
            String machineError;
            String selectedMachine = *App::cfg().machineName;
            if (!App::machine().selectByName(selectedMachine, &machineError)) {
                setStatusFail(machineError);
                Log::E(" === ERROR Machine Select: %s", machineError.c_str());
                Init::show();
                requestAbort(State::Type::NULL_STATE);
                return true;
            }
            return true;
        }

        setStatusFail("Ошибка загруки config"); //TODO надо сделать что бы при этой ошибке http был доступен и можно было исправить config
        Log::E(" === ERROR Core::config.load");
        Core::config.print_config();  // отладка - выведем что в config при ошибке
        Init::show();
        requestAbort(State::Type::NULL_STATE);
        return true;
    }

    // Подключает устройство к Wi-Fi при включенной настройке.
    static bool ConnectWiFi() {
        setStatus("Подключение к Wi-Fi");
        BootContext& boot = context();
        boot.wifiConnected = (*App::cfg().connectWifi == 1) && WiFiConfig::getInstance().begin();
        return true;
    }

    // Проверяет наличие обновлений ESP-прошивки.
    static bool UpdateESP() {
        setStatus("Проверка обновлений прошивки");
        if (!context().wifiConnected) return true;

        if ((*App::cfg().autoUpdate == 1) && (*App::cfg().updateEsp == 1)) {
            int level = ESPUpdate::getInstance().checkForUpdate();
            if (level > 0) {
                ESPUpdate::getInstance().FirmwareUpdate(level, [](int percent) {
                    Log::D("ESP update: %d%%", percent);
                });
            }
        }
        return true;
    }

    // Запрашивает у экрана версию UI и сохраняет ее в сервисе версий.
    static bool SetScreenVersion() {
        Screen& screen = Screen::getInstance();
        screen.updateScreenVersion();
        NexUpdate::getInstance().setCurrentVersion(screen.getScreenVersion());
        Log::L("Device version: %s", Version::makeDeviceVersion(NexUpdate::getInstance().getCurrentVersion()).c_str());
        return true;
    }

    // Запускает HTTP-сервер после подключения к сети.
    static bool StartHttp() {
        setStatus("Запуск HTTP сервера");
        if (context().wifiConnected && (*App::cfg().httpServer == 1)) {
            HServer::getInstance().begin();
        }
        return true;
    }

    // Подключает MQTT-клиент после появления сети.
    static bool ConnectMQTT() {
        setStatus("Подключение к серверу");
        if (context().wifiConnected) {
            MQTTc::getInstance().connect(); // подключится
        }
        return true;
    }

    // Загружает данные приложения из постоянного хранилища.
    static bool LoadData() {
        setStatus("Загрузка данных");
        // проверка лицензии
        /*
        if (!Licence::getInstance().isValid()) {
            pLoad::getInstance().text("Лицензия не верная. Работа не возможна!");
            Log::L(" === Лицензия не верная");
            requestAbort(State::Type::NULL_STATE);
            return true;
        }*/

        Core::data.load();
        Data::tuning.load();
        Data::profiles.load();
        Data::tasks.load();
        Stats::getInstance().init();
        return true;
    }

    // Инициализирует реестр устройств и проверяет его совместимость с машиной.
    static bool InitRegistry() {
        setStatus("Инициализация устройств");
        String registry_error_message;
        if (!App::reg().init(&registry_error_message)) { // Инициализация устройства
            setStatusFail(registry_error_message);
            Log::E(" === ERROR Registry Init: %s", registry_error_message.c_str());
            requestAbort(State::Type::NULL_STATE);
            return true;
        }

        // А теперь проверяем подходит ли то что загрузили к этому типу машины 
        Machine& machine = App::machine();
        String machineError;
        if (!machine.bindRegistry(App::reg(), &machineError)) {
            setStatusFail(machineError);
            Log::E(" === ERROR Machine Bind: %s", machineError.c_str());
            requestAbort(State::Type::NULL_STATE);
            return true;
        }

        MachineSpec::Report specReport = machine.validateRegistry(App::reg());
        for (const String& warning : specReport.warnings) {
            Log::D("%s", warning.c_str());
            App::diag().addWarning(State::ErrorCode::CONFIG_ERROR, "Конфигурация устройства неверная", warning);
        }
        if (!machine.shouldContinueBoot(*App::cfg().allowMissingHardware == 1)) {
            String failText = "Machine registry validation failed";
            if (!specReport.errors.empty()) {
                failText = specReport.errors.front();
            }
            setStatusFail(failText);
            for (const String& err : specReport.errors) {
                Log::E("%s", err.c_str());
            }
            requestAbort(State::Type::NULL_STATE);
            return true;
        }

        if (specReport.hasErrors()) {
            Log::E("[Machine] Registry validation has blocking errors, but ALLOW_MISSING_HARDWARE=1. Continue boot.");
            for (const String& err : specReport.errors) {
                Log::D("[Machine][warn-only] %s", err.c_str());
                App::diag().addWarning(State::ErrorCode::CONFIG_ERROR, "Работа с ограничениями", err);
            }
        }
        if (!machine.readyForMotion()) {
            Log::D("[Machine] Motion is not ready after registry binding.");
        }

        // ----
        Data::param.reset();
        App::mode() = Mode::NORMAL;
        App::diag().clearErrors();
        return true;
    }

    // Регистрирует все системные триггеры после инициализации.
    static bool RegisterTriggers() {
        setStatus("Регистрация триггеров");
        Trigger::getInstance().registerTrigger();
        McpTrigger::getInstance().init();
        return true;
    }

};
