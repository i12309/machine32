#pragma once
#include "Core.h"
#include "Catalog.h"
#include "Machine/Machine.h"
#include "Service/HServer.h"
#include "Service/NVS.h"
#include "Service/ESPUpdate.h"
#include "Service/NexUpdate.h"
#include "Service/Licence.h"
#include "Service/Stats.h"
#include "version.h"

#include "UI/Main/pLoad.h"
#include "UI/Main/pINFO.h"
#include "UI/Main/pINIT.h"
#include "State/State.h"
#include "Controller/McpTrigger.h"
#include "App/App.h"

class Boot : public State {
private:
    struct BootContext {
        bool wifiConnected = false;
        bool displayReady = false;
        bool abortRequested = false;
        bool haltRequested = false;
        State::Type abortState = State::Type::NULL_STATE;
        int label_num = 0;
    };

    static BootContext& context() {
        static BootContext ctx;
        return ctx;
    }

    static void resetContext() {
        context() = BootContext();
    }

    static void requestAbort(State::Type state) {
        BootContext& ctx = context();
        ctx.abortRequested = true;
        ctx.abortState = state;
    }

    static void requestHalt() {
        context().haltRequested = true;
    }

    static void setStatus(const String& text) {
        if (!context().displayReady) {
            // экран не работает - выводим в терминал
            Log::D("BOOT: %s", text.c_str());
            return;
        }

        BootContext& ctx = context();
        ctx.label_num++;
        pLoad& load = pLoad::getInstance();
        load.text(text);
        if (ctx.label_num-1 > 0) {
            uint32_t prevColor = load.getProgressColor(ctx.label_num-1);
            if (prevColor != Catalog::Color::red) {
                load.setProgressColor(ctx.label_num-1, Catalog::Color::green);
            }
        }
        load.setProgressColor(ctx.label_num, Catalog::Color::orange);
    }

    static void setStatusFail(const String& text) {
        if (!context().displayReady) {
            Log::E("BOOT: %s", text.c_str());
            return;
        }

        BootContext& ctx = context();
        pLoad& load = pLoad::getInstance();
        load.text(text);
        load.setProgressColor(ctx.label_num, Catalog::Color::red);
    }
public:
    Boot() : State(State::Type::BOOT) {}

    void oneRun() override {
        State::oneRun();
        resetContext();

        PlanManager& plan = App::plan();
        plan.beginPlan(this->type());

        plan.addAction(State::Type::ACTION, &Boot::LogStart, "LogStart");
        plan.addAction(State::Type::ACTION, &Boot::InitNextion, "InitNextion");
        plan.addAction(State::Type::ACTION, &Boot::ShowLoad, "ShowLoad");
        plan.addAction(State::Type::ACTION, &Boot::InitFileSystem, "InitFileSystem");
        plan.addAction(State::Type::ACTION, &Boot::InitNVS, "InitNVS");
        plan.addAction(State::Type::ACTION, &Boot::LoadConfig, "LoadConfig");
        plan.addAction(State::Type::ACTION, &Boot::ConnectWiFi, "ConnectWiFi");
        plan.addAction(State::Type::ACTION, &Boot::TryRecoverNextionIfMissing, "TryRecoverNextionIfMissing");
        plan.addAction(State::Type::ACTION, &Boot::UpdateESP, "UpdateESP");
        plan.addAction(State::Type::ACTION, &Boot::SetTFTVersion, "SetTFTVersion");
        plan.addAction(State::Type::ACTION, &Boot::UpdateTFT, "UpdateTFT");
        plan.addAction(State::Type::ACTION, &Boot::StartHttp, "StartHTTP");
        plan.addAction(State::Type::ACTION, &Boot::ConnectMQTT, "ConnectMQTT");
        plan.addAction(State::Type::ACTION, &Boot::LoadData, "LoadData");
        plan.addAction(State::Type::ACTION, &Boot::InitRegistry, "InitRegistry");
        plan.addAction(State::Type::ACTION, &Boot::RegisterTriggers, "RegisterTriggers");

    }

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
    static bool LogStart() {
        Log::L(" === START v.%s", Version::makeDeviceVersion(NexUpdate::getInstance().getCurrentVersion()).c_str());
        Log::D("ESP32 Chip: %s Rev %d", ESP.getChipModel(), ESP.getChipRevision());
        Log::D("Flash size: %d", ESP.getFlashChipSize());
        return true;
    }

    static bool InitNextion() {
        BootContext& boot = context();
        boot.displayReady = false;

        // три раза пытаемся запустить монитор
        for (int attempt = 1; attempt <= 3; ++attempt) {
            if (Page::nextionInit()) { // Инициализация экрана
                boot.displayReady = true;
                return true;
            }

            Log::D("Nextion init failed, attempt %d/3", attempt);
            if (attempt < 3) delay(300);
        }

        Log::E(" === ERROR Nextion Init ");
        return true;
    }

    static bool ShowLoad() {
        if (!context().displayReady) return true; // экрана нет - пропускаем
        pLoad::getInstance().show();
        setStatus("Загрузка интерфейса");
        pLoad::getInstance().checkVersion();
        return true;
    }

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

    static bool InitNVS() {
        setStatus("Инициализация NVS");
        NVS& nvs = NVS::getInstance();
        nvs.init();
        if (nvs.getInt("tft_rcnt", 0, "boot") > 2) nvs.setInt("tft_rcnt", 2, "boot");
        if (context().displayReady && nvs.getInt("tft_rcnt", 0, "boot") != 0) nvs.setInt("tft_rcnt", 0, "boot");

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
            pINFO::showInfo("", "Что-то пошло не так!", "Проверьте параметры", [](){pINIT::getInstance().show();});
            requestAbort(State::Type::NULL_STATE);
            return true;
        }
        nvs.setInt("boot_count", bootCount, "boot");
        return true;
    }

    static bool LoadConfig() {
        setStatus("Загрузка конфигурации");
        if (Core::config.load(!context().displayReady)) {
            String machineError;
            String selectedMachine = *App::cfg().machineName;
            if (!App::machine().selectByName(selectedMachine, &machineError)) {
                setStatusFail(machineError);
                Log::E(" === ERROR Machine Select: %s", machineError.c_str());
                pINIT::getInstance().show();
                requestAbort(State::Type::NULL_STATE);
                return true;
            }
            return true;
        }

        setStatusFail("Ошибка загруки config"); //TODO надо сделать что бы при этой ошибке http был доступен и можно было исправить config
        Log::E(" === ERROR Core::config.load");
        Core::config.print_config();  // отладка - выведем что в config при ошибке
        pINIT::getInstance().show();
        requestAbort(State::Type::NULL_STATE);
        return true;
    }

    static bool ConnectWiFi() {
        setStatus("Подключение к Wi-Fi");
        BootContext& boot = context();
        boot.wifiConnected = (*App::cfg().connectWifi == 1) && WiFiConfig::getInstance().begin();
        return true;
    }

    static bool TryRecoverNextionIfMissing() {
        setStatus("Восстановление интерфейса");
        if (context().displayReady) return true; // а экран то работает
        if (!context().wifiConnected) return true; // а связи нет

        NVS& nvs = NVS::getInstance();
        int tftRecoveryCount = nvs.getInt("tft_rcnt", 0, "boot");
        if (tftRecoveryCount >= 2) return true;

        int nextAttempt = tftRecoveryCount + 1;
        nvs.setInt("tft_rcnt", nextAttempt, "boot");
        Log::L("Попытка восстановить экран %d/2", nextAttempt);

        int level = NexUpdate::getInstance().checkForUpdate();
        if (level <= 0) {
            level = 2;
            Log::D("No newer TFT version found. Using forced recovery level %d", level);
        }

        nvs.setInt("tft_rcnt", 0, "boot"); // TODO - тут стоит подумать может все таки это поместить внутри upload

        if (NexUpdate::getInstance().upload(level, true)) {
            Log::L("Nextion recovery success on attempt %d/2. Rebooting...", nextAttempt);
            requestHalt();
            return true;
        }

        Log::E("Nextion recovery failed on attempt %d/2", nextAttempt);
        return true;
    }

    static bool UpdateESP() {
        setStatus("Проверка обновлений прошивки");
        if (!context().wifiConnected) return true;

        if ((*App::cfg().autoUpdate == 1) && (*App::cfg().updateEsp == 1)) {
            int level = ESPUpdate::getInstance().checkForUpdate();
            if (level > 0) {
                ESPUpdate::getInstance().FirmwareUpdate(level, [](int percent) {
                    String text = "Обновление: " + String(percent) + "%";
                    pLoad::getInstance().text(text);
                });
            }
        }
        return true;
    }

    static bool SetTFTVersion() {
        if (!context().displayReady) {
            NexUpdate::getInstance().setCurrentVersion(-1);
            return true;
        }

        setStatus("Проверка версии экрана");
        NexUpdate::getInstance().setCurrentVersion(pLoad::getInstance().getHMIVersion());
        Log::L("Device version: %s", Version::makeDeviceVersion(NexUpdate::getInstance().getCurrentVersion()).c_str());
        return true;
    }

    static bool UpdateTFT() {
        setStatus("Обновление интерфейса");
        if (!context().wifiConnected) return true;

        if ((*App::cfg().autoUpdate == 1) && (*App::cfg().updateTft == 1)) {
            int level = NexUpdate::getInstance().checkForUpdate();
            if (level > 0) {
                if (context().displayReady) pLoad::getInstance().text("Обновление интерфейса");
                if (NexUpdate::getInstance().upload(level, true)) {
                    requestHalt();
                    return true;
                }
                Log::E("Nextion update failed in boot flow");
            }
        }
        return true;
    }

    static bool StartHttp() {
        setStatus("Запуск HTTP сервера");
        if (context().wifiConnected && (*App::cfg().httpServer == 1)) {
            HServer::getInstance().begin();
        }
        return true;
    }

    static bool ConnectMQTT() {
        setStatus("Подключение к серверу");
        if (context().wifiConnected) {
            MQTTc::getInstance().connect(); // подключится
        }
        return true;
    }

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

    static bool RegisterTriggers() {
        setStatus("Регистрация триггеров");
        Trigger::getInstance().registerTrigger();
        McpTrigger::getInstance().init();
        return true;
    }

};

