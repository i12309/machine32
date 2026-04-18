#include "App.h"

#include "Core.h"
#include "Machine/Machine.h"
#include "Controller/Registry.h"
#include "Controller/Trigger.h"
#include "Service/DeviceError.h"
#include "Service/Log.h"
#include "Service/Service.h"
#include "State/PlanManager.h"
#include "State/Scene.h"
#include "State/State.h"
#include "Screen/Screen.h"
#include "UI/Page.h"

using machine32::screen::Screen;

namespace {
App::Context buildAppContext() {
    App::Context ctx;

    auto& config = Core::config;
    auto& settings = Core::settings;
    auto& data = Core::data;

    ctx.config.machineName = &config.machine;
    ctx.config.allowMissingHardware = &settings.ALLOW_MISSING_HARDWARE;
    ctx.config.checkSystem = &settings.CHECK_SYSTEM;
    ctx.config.connectWifi = &settings.CONNECT_WIFI;
    ctx.config.httpServer = &settings.HTTP_SERVER;
    ctx.config.webEnabled = &settings.WEB;
    ctx.config.autoUpdate = &settings.AUTO_UPDATE;
    ctx.config.updateEsp = &settings.UPDATE;
    ctx.config.updateTft = &settings.TFT_UPDATE;

    ctx.storage.dataDoc = &data.doc;

    Machine& machine = Machine::getInstance();
    ctx.machine.machine = &machine;
    ctx.machine.devices = &machine.context();
    ctx.machine.registry = &Registry::getInstance();

    ctx.plan.manager = &PlanManager::instance();
    ctx.diagnostics.deviceError = &DeviceError::getInstance();
    ctx.motion.scene = &Scene::getInstance();

    ctx.ui.activePage = &Page::activePage;
    ctx.ui.previousPage = &Page::previousPage;

    return ctx;
}
}  // namespace

App* App::instance_ = nullptr;

App::App() : context_(buildAppContext()) {
    instance_ = this;
}

App::Context::ConfigContext& App::cfg() {
    App::Context* ctx = App::tryContext();
    if (ctx == nullptr) {
        Log::E("[App] Context is not initialized. Aborting.");
        abort();
    }
    return ctx->config;
}

IMachineContext& App::ctx() {
    App::Context* ctx = App::tryContext();
    if (ctx == nullptr) {
        Log::E("[App] Context is not initialized. Aborting.");
        abort();
    }
    IMachineContext* deviceContext = ctx->machine.devices;
    if (deviceContext != nullptr) return *deviceContext;
    return Machine::getInstance().context();
}

Machine& App::machine() {
    App::Context* ctx = App::tryContext();
    if (ctx == nullptr) {
        Log::E("[App] Context is not initialized. Aborting.");
        abort();
    }
    Machine* m = ctx->machine.machine;
    if (m != nullptr) return *m;
    return Machine::getInstance();
}

Registry& App::reg() {
    App::Context* ctx = App::tryContext();
    if (ctx == nullptr) {
        Log::E("[App] Context is not initialized. Aborting.");
        abort();
    }
    Registry* reg = ctx->machine.registry;
    if (reg != nullptr) return *reg;
    return Registry::getInstance();
}

Scene& App::scene() {
    App::Context* ctx = App::tryContext();
    if (ctx == nullptr) {
        Log::E("[App] Context is not initialized. Aborting.");
        abort();
    }
    Scene* scene = ctx->motion.scene;
    if (scene != nullptr) return *scene;
    return Scene::getInstance();
}

DeviceError& App::diag() {
    App::Context* ctx = App::tryContext();
    if (ctx == nullptr) {
        Log::E("[App] Context is not initialized. Aborting.");
        abort();
    }
    DeviceError* diagnostics = ctx->diagnostics.deviceError;
    if (diagnostics != nullptr) return *diagnostics;
    return DeviceError::getInstance();
}

State*& App::state() {
    App::Context* ctx = App::tryContext();
    if (ctx == nullptr) {
        Log::E("[App] Context is not initialized. Aborting.");
        abort();
    }
    return ctx->runtime.currentState;
}

Catalog::Mode& App::mode() {
    App::Context* ctx = App::tryContext();
    if (ctx == nullptr) {
        Log::E("[App] Context is not initialized. Aborting.");
        abort();
    }
    return ctx->runtime.mode;
}

PlanManager& App::plan() {
    App::Context* ctx = App::tryContext();
    if (ctx == nullptr) {
        Log::E("[App] Context is not initialized. Aborting.");
        abort();
    }
    PlanManager* manager = ctx->plan.manager;
    if (manager != nullptr) return *manager;

    Log::E("[App] PlanManager is not initialized. Aborting.");
    abort();
}

void App::init() {
    Log::init();
    State::init();
    Trigger::init();
}

void App::process() {
    Screen::getInstance().tick();
    Page::process();
    State::process();
    Service::process();
}
