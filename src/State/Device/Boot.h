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

// РЎРѕСЃС‚РѕСЏРЅРёРµ РЅР°С‡Р°Р»СЊРЅРѕР№ Р·Р°РіСЂСѓР·РєРё СѓСЃС‚СЂРѕР№СЃС‚РІР°.
class Boot : public State {
private:
    // РљРѕРЅС‚РµРєСЃС‚ Р·Р°РіСЂСѓР·РєРё, РєРѕС‚РѕСЂС‹Р№ Р¶РёРІРµС‚ РјРµР¶РґСѓ С€Р°РіР°РјРё boot-РїР»Р°РЅР°.
    struct BootContext {
        bool wifiConnected = false;
        bool screenReady = false;
        bool waitingInfo = false;
        bool abortRequested = false;
        bool haltRequested = false;
        State::Type abortState = State::Type::NULL_STATE;
    };

    // Р’РѕР·РІСЂР°С‰Р°РµС‚ РѕР±С‰РёР№ РєРѕРЅС‚РµРєСЃС‚ С‚РµРєСѓС‰РµР№ Р·Р°РіСЂСѓР·РєРё.
    static BootContext& context() {
        static BootContext ctx;
        return ctx;
    }

    // РЎР±СЂР°СЃС‹РІР°РµС‚ РєРѕРЅС‚РµРєСЃС‚ РїРµСЂРµРґ РЅРѕРІС‹Рј Р·Р°РїСѓСЃРєРѕРј BOOT.
    static void resetContext() {
        context() = BootContext();
    }

    // Р—Р°РїСЂР°С€РёРІР°РµС‚ Р°РІР°СЂРёР№РЅС‹Р№ РІС‹С…РѕРґ РёР· BOOT РІ СѓРєР°Р·Р°РЅРЅРѕРµ СЃРѕСЃС‚РѕСЏРЅРёРµ.
    static void requestAbort(State::Type state) {
        BootContext& ctx = context();
        ctx.abortRequested = true;
        ctx.abortState = state;
    }

    // Р—Р°РїСЂР°С€РёРІР°РµС‚ РѕСЃС‚Р°РЅРѕРІРєСѓ BOOT Р±РµР· РїРµСЂРµС…РѕРґР° РІ РґСЂСѓРіРѕРµ СЃРѕСЃС‚РѕСЏРЅРёРµ.
    static void requestHalt() {
        context().haltRequested = true;
    }

    // РџРёС€РµС‚ СЃР»СѓР¶РµР±РЅС‹Р№ СЃС‚Р°С‚СѓСЃ Р·Р°РіСЂСѓР·РєРё РІ Р»РѕРі.
    static void setStatus(const String& text) {
        Log::D("BOOT: %s", text.c_str());
    }

    // РџРёС€РµС‚ РѕС€РёР±РѕС‡РЅС‹Р№ СЃС‚Р°С‚СѓСЃ Р·Р°РіСЂСѓР·РєРё РІ Р»РѕРі.
    static void setStatusFail(const String& text) {
        Log::E("BOOT: %s", text.c_str());
    }
public:
    // РЎРѕР·РґР°РµС‚ СЃРѕСЃС‚РѕСЏРЅРёРµ BOOT.
    Boot() : State(State::Type::BOOT) {}

    // Р¤РѕСЂРјРёСЂСѓРµС‚ РїРѕС€Р°РіРѕРІС‹Р№ РїР»Р°РЅ Р·Р°РіСЂСѓР·РєРё СѓСЃС‚СЂРѕР№СЃС‚РІР°.
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

    // Р’С‹РїРѕР»РЅСЏРµС‚ boot-РїР»Р°РЅ Рё СѓРґРµСЂР¶РёРІР°РµС‚ BOOT, РїРѕРєР° РѕС‚РєСЂС‹С‚ РЅРѕРІС‹Р№ СЌРєСЂР°РЅ Info.
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
                setStatus("РџСЂРѕРІРµСЂРєР° СѓСЃС‚СЂРѕР№СЃС‚РІР°");
                return Factory(State::Type::CHECK);
            }
            ESPUpdate::getInstance().markCurrentFirmwareValid(); // Р•СЃР»Рё РЅРµ CHECK_SYSTEM С‚Рѕ СЃС‡РёС‚Р°РµРј С‡С‚Рѕ Р·Р°РіСЂСѓР·РєР° РїСЂРѕС€Р»Р° СѓСЃРїРµС€РЅРѕ Рё РІР°Р»РёРґРёСЂСѓРµРј РЅРѕРІСѓСЋ РїСЂРѕС€РёРІРєСѓ, С‡С‚Рѕ Р±С‹ РїСЂРё СЃР»РµРґСѓСЋС‰РµР№ Р·Р°РіСЂСѓР·РєРё РЅРµ Р±С‹Р»Рѕ РїСЂРѕРІРµСЂРєРё Рё РІРѕР·РјРѕР¶РЅРѕРіРѕ РѕС‚РєР°С‚Р°.
            return Factory(State::Type::IDLE);
        }

        return Factory(plan.nextType(this->type()));
    }

private:
    // РџРёС€РµС‚ СЃС‚Р°СЂС‚РѕРІСѓСЋ СЃР»СѓР¶РµР±РЅСѓСЋ РёРЅС„РѕСЂРјР°С†РёСЋ Рѕ РєРѕРЅС‚СЂРѕР»Р»РµСЂРµ.
    static bool LogStart() {
        Log::L(" === START v.%s", Version::makeDeviceVersion(NexUpdate::getInstance().getCurrentVersion()).c_str());
        Log::D("ESP32 Chip: %s Rev %d", ESP.getChipModel(), ESP.getChipRevision());
        Log::D("Flash size: %d", ESP.getFlashChipSize());
        return true;
    }

    // РРЅРёС†РёР°Р»РёР·РёСЂСѓРµС‚ РЅРѕРІС‹Р№ СЌРєСЂР°РЅРЅС‹Р№ runtime.
    static bool InitScreen() {
        BootContext& boot = context();
        boot.screenReady = false;

        Screen& screen = Screen::getInstance();
        if (!screen.init(0)) {
            Log::E(" === ERROR Screen Init: %s", screen.lastError());
            requestAbort(State::Type::NULL_STATE);
            return true;
        }

        boot.screenReady = !screen.isSilent();
        return true;
    }

    // РџРѕРєР°Р·С‹РІР°РµС‚ СЌРєСЂР°РЅ Р·Р°РіСЂСѓР·РєРё С‡РµСЂРµР· РЅРѕРІС‹Р№ РєР»Р°СЃСЃ СЃС‚СЂР°РЅРёС†С‹.
    static bool ShowLoad() {
        if (!context().screenReady) return true;

        setStatus("Р—Р°РіСЂСѓР·РєР° РёРЅС‚РµСЂС„РµР№СЃР°");
        if (!Load::show()) {
            Log::E(" === ERROR ShowLoad: %s", Screen::getInstance().lastError());
            requestAbort(State::Type::NULL_STATE);
        }
        return true;
    }

    // РРЅРёС†РёР°Р»РёР·РёСЂСѓРµС‚ С„Р°Р№Р»РѕРІСѓСЋ СЃРёСЃС‚РµРјСѓ СѓСЃС‚СЂРѕР№СЃС‚РІР°.
    static bool InitFileSystem() {
        setStatus("РРЅРёС†РёР°Р»РёР·Р°С†РёСЏ С„Р°Р№Р»РѕРІРѕР№ СЃРёСЃС‚РµРјС‹");
        if (FileSystem::init(true)) {
            return true;
        }

        setStatusFail("РћС€РёР±РєР° С„Р°Р№Р»РѕРІРѕР№ СЃРёСЃС‚РµРјС‹");
        Log::E(" === ERROR FileSystem Init ");
        requestAbort(State::Type::NULL_STATE);
        return true;
    }

    // РРЅРёС†РёР°Р»РёР·РёСЂСѓРµС‚ NVS Рё РѕР±СЂР°Р±Р°С‚С‹РІР°РµС‚ СЃС‡РµС‚С‡РёРє РЅРµСѓРґР°С‡РЅС‹С… Р·Р°РіСЂСѓР·РѕРє.
    static bool InitNVS() {
        setStatus("РРЅРёС†РёР°Р»РёР·Р°С†РёСЏ NVS");
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
            Info::showInfo("", "Р§С‚Рѕ-С‚Рѕ РїРѕС€Р»Рѕ РЅРµ С‚Р°Рє!", "РџСЂРѕРІРµСЂСЊС‚Рµ РїР°СЂР°РјРµС‚СЂС‹", []() { Init::show(); });
            requestAbort(State::Type::NULL_STATE);
            return true;
        }
        nvs.setInt("boot_count", bootCount, "boot");
        return true;
    }

    // Р—Р°РіСЂСѓР¶Р°РµС‚ РєРѕРЅС„РёРіСѓСЂР°С†РёСЋ Рё РІС‹Р±РёСЂР°РµС‚ С‚РёРї РјР°С€РёРЅС‹.
    static bool LoadConfig() {
        setStatus("Р—Р°РіСЂСѓР·РєР° РєРѕРЅС„РёРіСѓСЂР°С†РёРё");
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

        setStatusFail("РћС€РёР±РєР° Р·Р°РіСЂСѓРєРё config"); //TODO РЅР°РґРѕ СЃРґРµР»Р°С‚СЊ С‡С‚Рѕ Р±С‹ РїСЂРё СЌС‚РѕР№ РѕС€РёР±РєРµ http Р±С‹Р» РґРѕСЃС‚СѓРїРµРЅ Рё РјРѕР¶РЅРѕ Р±С‹Р»Рѕ РёСЃРїСЂР°РІРёС‚СЊ config
        Log::E(" === ERROR Core::config.load");
        Core::config.print_config();  // РѕС‚Р»Р°РґРєР° - РІС‹РІРµРґРµРј С‡С‚Рѕ РІ config РїСЂРё РѕС€РёР±РєРµ
        Init::show();
        requestAbort(State::Type::NULL_STATE);
        return true;
    }

    // РџРѕРґРєР»СЋС‡Р°РµС‚ СѓСЃС‚СЂРѕР№СЃС‚РІРѕ Рє Wi-Fi РїСЂРё РІРєР»СЋС‡РµРЅРЅРѕР№ РЅР°СЃС‚СЂРѕР№РєРµ.
    static bool ConnectWiFi() {
        setStatus("РџРѕРґРєР»СЋС‡РµРЅРёРµ Рє Wi-Fi");
        BootContext& boot = context();
        boot.wifiConnected = (*App::cfg().connectWifi == 1) && WiFiConfig::getInstance().begin();
        return true;
    }

    // РџСЂРѕРІРµСЂСЏРµС‚ РЅР°Р»РёС‡РёРµ РѕР±РЅРѕРІР»РµРЅРёР№ ESP-РїСЂРѕС€РёРІРєРё.
    static bool UpdateESP() {
        setStatus("РџСЂРѕРІРµСЂРєР° РѕР±РЅРѕРІР»РµРЅРёР№ РїСЂРѕС€РёРІРєРё");
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

    // Р—Р°РїСЂР°С€РёРІР°РµС‚ Сѓ СЌРєСЂР°РЅР° РІРµСЂСЃРёСЋ UI Рё СЃРѕС…СЂР°РЅСЏРµС‚ РµРµ РІ СЃРµСЂРІРёСЃРµ РІРµСЂСЃРёР№.
    static bool SetScreenVersion() {
        Screen& screen = Screen::getInstance();
        screen.updateScreenVersion();
        NexUpdate::getInstance().setCurrentVersion(screen.getScreenVersion());
        Log::L("Device version: %s", Version::makeDeviceVersion(NexUpdate::getInstance().getCurrentVersion()).c_str());
        return true;
    }

    // Р—Р°РїСѓСЃРєР°РµС‚ HTTP-СЃРµСЂРІРµСЂ РїРѕСЃР»Рµ РїРѕРґРєР»СЋС‡РµРЅРёСЏ Рє СЃРµС‚Рё.
    static bool StartHttp() {
        setStatus("Р—Р°РїСѓСЃРє HTTP СЃРµСЂРІРµСЂР°");
        if (context().wifiConnected && (*App::cfg().httpServer == 1)) {
            HServer::getInstance().begin();
        }
        return true;
    }

    // РџРѕРґРєР»СЋС‡Р°РµС‚ MQTT-РєР»РёРµРЅС‚ РїРѕСЃР»Рµ РїРѕСЏРІР»РµРЅРёСЏ СЃРµС‚Рё.
    static bool ConnectMQTT() {
        setStatus("РџРѕРґРєР»СЋС‡РµРЅРёРµ Рє СЃРµСЂРІРµСЂСѓ");
        if (context().wifiConnected) {
            MQTTc::getInstance().connect(); // РїРѕРґРєР»СЋС‡РёС‚СЃСЏ
        }
        return true;
    }

    // Р—Р°РіСЂСѓР¶Р°РµС‚ РґР°РЅРЅС‹Рµ РїСЂРёР»РѕР¶РµРЅРёСЏ РёР· РїРѕСЃС‚РѕСЏРЅРЅРѕРіРѕ С…СЂР°РЅРёР»РёС‰Р°.
    static bool LoadData() {
        setStatus("Р—Р°РіСЂСѓР·РєР° РґР°РЅРЅС‹С…");
        // РїСЂРѕРІРµСЂРєР° Р»РёС†РµРЅР·РёРё
        /*
        if (!Licence::getInstance().isValid()) {
            pLoad::getInstance().text("Р›РёС†РµРЅР·РёСЏ РЅРµ РІРµСЂРЅР°СЏ. Р Р°Р±РѕС‚Р° РЅРµ РІРѕР·РјРѕР¶РЅР°!");
            Log::L(" === Р›РёС†РµРЅР·РёСЏ РЅРµ РІРµСЂРЅР°СЏ");
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

    // РРЅРёС†РёР°Р»РёР·РёСЂСѓРµС‚ СЂРµРµСЃС‚СЂ СѓСЃС‚СЂРѕР№СЃС‚РІ Рё РїСЂРѕРІРµСЂСЏРµС‚ РµРіРѕ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚СЊ СЃ РјР°С€РёРЅРѕР№.
    static bool InitRegistry() {
        setStatus("РРЅРёС†РёР°Р»РёР·Р°С†РёСЏ СѓСЃС‚СЂРѕР№СЃС‚РІ");
        String registry_error_message;
        if (!App::reg().init(&registry_error_message)) { // РРЅРёС†РёР°Р»РёР·Р°С†РёСЏ СѓСЃС‚СЂРѕР№СЃС‚РІР°
            setStatusFail(registry_error_message);
            Log::E(" === ERROR Registry Init: %s", registry_error_message.c_str());
            requestAbort(State::Type::NULL_STATE);
            return true;
        }

        // Рђ С‚РµРїРµСЂСЊ РїСЂРѕРІРµСЂСЏРµРј РїРѕРґС…РѕРґРёС‚ Р»Рё С‚Рѕ С‡С‚Рѕ Р·Р°РіСЂСѓР·РёР»Рё Рє СЌС‚РѕРјСѓ С‚РёРїСѓ РјР°С€РёРЅС‹ 
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
            App::diag().addWarning(State::ErrorCode::CONFIG_ERROR, "РљРѕРЅС„РёРіСѓСЂР°С†РёСЏ СѓСЃС‚СЂРѕР№СЃС‚РІР° РЅРµРІРµСЂРЅР°СЏ", warning);
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
                App::diag().addWarning(State::ErrorCode::CONFIG_ERROR, "Р Р°Р±РѕС‚Р° СЃ РѕРіСЂР°РЅРёС‡РµРЅРёСЏРјРё", err);
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

    // Р РµРіРёСЃС‚СЂРёСЂСѓРµС‚ РІСЃРµ СЃРёСЃС‚РµРјРЅС‹Рµ С‚СЂРёРіРіРµСЂС‹ РїРѕСЃР»Рµ РёРЅРёС†РёР°Р»РёР·Р°С†РёРё.
    static bool RegisterTriggers() {
        setStatus("Р РµРіРёСЃС‚СЂР°С†РёСЏ С‚СЂРёРіРіРµСЂРѕРІ");
        Trigger::getInstance().registerTrigger();
        McpTrigger::getInstance().init();
        return true;
    }

};
