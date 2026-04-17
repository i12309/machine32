#include "ScGuard.h"

#include <cstdlib>

#include "App/App.h"
#include "Controller/Trigger.h"
#include "Service/Log.h"

namespace scene {

String ScGuard::motorName(IStepper* motor) const {
    // Преобразует указатель мотора в имя-ключ для map.
    if (motor == nullptr) return "";
    const std::string& nameStd = motor->getMotorName();
    if (nameStd.empty()) return "";
    return String(nameStd.c_str());
}

void ScGuard::report(const GuardContext& ctx, const String& shortDetails) const {
    // Пишет ошибку в диагностику с нужной критичностью.
    switch (ctx.kind) {
        case DeviceError::Kind::Fatal:
            App::diag().addFatal(ctx.errorCode, "", shortDetails);
            break;
        case DeviceError::Kind::Error:
            App::diag().addError(ctx.errorCode, "", shortDetails);
            break;
        case DeviceError::Kind::Warning:
            App::diag().addWarning(ctx.errorCode, shortDetails);
            break;
    }
}

void ScGuard::activateWatchdogFor(IStepper* motor) const {
    // Автоматически включаем watchdog для поддерживаемых моторов.
    if (motor == App::ctx().mTable) {
        Trigger::getInstance().activateTrigger("TABLE_TIMEOUT");
    } else if (motor == App::ctx().mGuillotine) {
        Trigger::getInstance().activateTrigger("GUILLOTINE_TIMEOUT");
    }
}

void ScGuard::deactivateWatchdogFor(IStepper* motor) const {
    // Автоматически выключаем watchdog для поддерживаемых моторов.
    if (motor == App::ctx().mTable) {
        Trigger::getInstance().deactivateTrigger("TABLE_TIMEOUT");
    } else if (motor == App::ctx().mGuillotine) {
        Trigger::getInstance().deactivateTrigger("GUILLOTINE_TIMEOUT");
    }
}

bool ScGuard::run(IStepper* motor, Catalog::ErrorCode errorCode, DeviceError::Kind kind) {
    // run всегда делает reset перед start.
    reset(motor);
    if (!start(motor, errorCode, kind)) return false;
    activateWatchdogFor(motor);
    return true;
}

void ScGuard::reset(IStepper* motor) {
    // Сбрасывает контекст guard для одного мотора.
    const String name = motorName(motor);
    if (name.length() == 0) return;

    GuardContext& ctx = states_[name];
    ctx.active = false;
    ctx.errorCode = Catalog::ErrorCode::NO_ERROR;
    ctx.kind = DeviceError::Kind::Fatal;
    ctx.startMs = 0;
    ctx.startPosition = 0;
}

void ScGuard::stop(IStepper* motor) {
    // Останавливает guard: чистит состояние и снимает watchdog.
    reset(motor);
    deactivateWatchdogFor(motor);
}

void ScGuard::resetAll() {
    // Полный сброс всех состояний guard.
    states_.clear();
}

bool ScGuard::start(IStepper* motor, Catalog::ErrorCode errorCode, DeviceError::Kind kind) {
    // start валидирует входные данные и стартует контекст.
    if (motor == nullptr || !motor->isRunning()) return false;
    if (errorCode == Catalog::ErrorCode::NO_ERROR) return false;

    const String timeoutName = Catalog::TimeoutName(errorCode);
    if (timeoutName.length() == 0) return false;

    const String name = motorName(motor);
    if (name.length() == 0) return false;

    GuardContext& ctx = states_[name];
    ctx.active = true;
    ctx.errorCode = errorCode;
    ctx.kind = kind;
    ctx.startMs = millis();
    ctx.startPosition = motor->getCurrentPosition();
    return true;
}

bool ScGuard::check(IStepper* motor) {
    // check проверяет превышение лимитов для активного мотора.
    if (motor == nullptr) return false;

    const String name = motorName(motor);
    if (name.length() == 0) return false;

    auto it = states_.find(name);
    if (it == states_.end()) return false;
    GuardContext& ctx = it->second;
    if (!ctx.active) return false;

    // Если мотор уже остановился, контроль не нужен.
    if (!motor->isRunning()) {
        reset(motor);
        return false;
    }

    const String timeoutName = Catalog::TimeoutName(ctx.errorCode);
    if (timeoutName.length() == 0) {
        reset(motor);
        return false;
    }

    const int32_t maxSteps = motor->getCheck(timeoutName.c_str(), -1);
    const int32_t maxMs = motor->getCheckMs(timeoutName.c_str(), -1);
    if (maxSteps <= 0 && maxMs <= 0) return false;

    const uint32_t elapsedMs = millis() - ctx.startMs;
    const int32_t elapsedSteps = abs(motor->getCurrentPosition() - ctx.startPosition);

    const bool stepsHit = (maxSteps > 0 && elapsedSteps >= maxSteps);
    const bool msHit = (maxMs > 0 && elapsedMs >= static_cast<uint32_t>(maxMs));
    if (!stepsHit && !msHit) return false;

    // Формируем подробную строку для лога и UI.
    const String longDetails = "Motor=" + name +
                               ", mode=" + timeoutName +
                               ", steps=" + String(elapsedSteps) + "/" + String(maxSteps) +
                               ", ms=" + String(elapsedMs) + "/" + String(maxMs);
    const String shortDetails = "Превышен guard для " + name;

    report(ctx, shortDetails);
    Log::D("Guard timeout: %s", longDetails.c_str());
    reset(motor);
    return true;
}

}  // namespace scene
