#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "Catalog.h"

class Machine;
class IMachineContext;
class Registry;
class Scene;
class DeviceError;
class State;
class Page;
class PlanManager;

class App {
public:
    // Composition root data container kept inside App to avoid extra entities.
    struct Context {
        struct ConfigContext {
            String* machineName = nullptr;
            int* allowMissingHardware = nullptr;
            int* checkSystem = nullptr;
            int* connectWifi = nullptr;
            int* httpServer = nullptr;
            int* webEnabled = nullptr;
            int* autoUpdate = nullptr;
            int* updateEsp = nullptr;
            int* updateTft = nullptr;
        };

        struct StorageContext {
            JsonDocument* dataDoc = nullptr;
        };

        struct MachineContext {
            Machine* machine = nullptr;
            IMachineContext* devices = nullptr;
            Registry* registry = nullptr;
        };

        struct RuntimeContext {
            State* currentState = nullptr;
            Catalog::Mode mode = Catalog::Mode::NORMAL;
        };

        struct PlanContext {
            PlanManager* manager = nullptr;
        };

        struct DiagnosticsContext {
            DeviceError* deviceError = nullptr;
        };

        struct MotionContext {
            Scene* scene = nullptr;
        };

        struct UiContext {
            Page** activePage = nullptr;
            Page** previousPage = nullptr;
        };

        ConfigContext config;
        StorageContext storage;
        MachineContext machine;
        RuntimeContext runtime;
        PlanContext plan;
        DiagnosticsContext diagnostics;
        MotionContext motion;
        UiContext ui;
    };

    App();
    ~App() = default;

    static App* instance() { return instance_; }
    static Context* tryContext() {
        return instance_ ? &instance_->context_ : nullptr;
    }
    static IMachineContext& ctx();
    static Context::ConfigContext& cfg();
    static Machine& machine();
    static Registry& reg();
    static Scene& scene();
    static DeviceError& diag();
    static State*& state();
    static Catalog::Mode& mode();
    static PlanManager& plan();

    void init();
    void process();

    Context& context() { return context_; }
    const Context& context() const { return context_; }

private:
    static App* instance_;
    Context context_;
};
