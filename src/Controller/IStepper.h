#pragma once

#include <FastAccelStepper.h>

#include <cmath>
#include <string>

#include "I2CBus.h"
#include "IStepperParam.h"
#include "MCP.h"
#include "Data.h"
#include "Service/Log.h"
#include "Service/Stats.h"

class IStepper {
    FastAccelStepper* stepper = nullptr;
    int stepPin;
    int dirPin;
    int enaPin;
    bool enableDriver = false;
    bool _enabled = true;

    IStepperParam _param;

    std::string motorName;
    bool _continuousRun = false;
    int32_t _continuousStep = 0;

    // Позиция, захваченная из ISR датчика — минимальная задержка относительно события.
    volatile int32_t _capturedPos = 0;

    bool ready() const {
        return _enabled && stepper != nullptr;
    }

    void startContinuousRun() {
        if (!ready()) return;
        _continuousRun = true;
        _continuousStep = stepper->getCurrentPosition();
    }

    void finishContinuousRun(bool includeStepsToStop = false) {
        if (!ready() || !_continuousRun) return;
        int32_t pos = stepper->getCurrentPosition();
        if (includeStepsToStop) {
            pos += stepper->stepsToStop();
        }
        int32_t delta = pos - _continuousStep;
        if (delta != 0) Stats::getInstance().onMotorSteps(motorName.c_str(), delta);
        _continuousRun = false;
    }

public:
    void setMotorName(const std::string& name) {
        motorName = name;
    }

    const std::string& getMotorName() const {
        return motorName;
    }

    IStepper(int _stepPin, int _dirPin, int _enaPin, FastAccelStepperEngine& engine, MCP* mcp = nullptr, bool enabled = true)
        : stepPin(_stepPin), dirPin(_dirPin), enaPin(_enaPin), _enabled(enabled) {
        if (!_enabled) return;

        stepper = engine.stepperConnectToPin(stepPin, FasDriver::RMT); // выбор драйвера MCPWM_PCNT
        if (!stepper) stepper = engine.stepperConnectToPin(stepPin, FasDriver::RMT); // выбор драйвера RMT

        if (!stepper) {
            _enabled = false;
            Log::E("[Stepper] stepperConnectToPin failed for pin=%d", stepPin);
            return;
        }

        if (mcp != nullptr && mcp->IO() != nullptr) {
            const int mcpDirPin = dirPin;
            const int mcpEnaPin = enaPin;
            if (mcpDirPin < 0 || mcpDirPin > 15 || mcpEnaPin < 0 || mcpEnaPin > 15) {
                _enabled = false;
                Log::E("[Stepper] invalid MCP pins dir=%d ena=%d", mcpDirPin, mcpEnaPin);
                return;
            }
            if (I2CBus::lock()) {
                mcp->pinMode(static_cast<uint8_t>(mcpDirPin), OUTPUT);
                mcp->pinMode(static_cast<uint8_t>(mcpEnaPin), OUTPUT);
                I2CBus::unlock();
            }

            const uint8_t mcpBase = static_cast<uint8_t>((mcp->getNumber() & 0x0F) << 4);
            dirPin = static_cast<int>(PIN_EXTERNAL_FLAG | mcpBase | (mcpDirPin & 0x0F));
            enaPin = static_cast<int>(PIN_EXTERNAL_FLAG | mcpBase | (mcpEnaPin & 0x0F));
        }

        stepper->setDirectionPin(static_cast<uint8_t>(dirPin));
        ENA(true);
    }


    void move(int32_t steps, bool blocking = false) {
        if (!ready()) return;
        finishContinuousRun(false);
        stepper->move(steps, blocking);
        if (steps != 0) Stats::getInstance().onMotorSteps(motorName.c_str(), steps);
    }

    void moveTo(int32_t target, bool blocking = false) {
        if (!ready()) return;
        // Для moveTo не считаем статистику как "командные шаги":
        // при перенацеливании на лету (по датчику) это дало бы ложный перерасход.
        finishContinuousRun(false);
        stepper->moveTo(target, blocking);
    }

    void moveMM(float mm, float ration, bool blocking = false) {
        if (!ready()) return;
        finishContinuousRun(false);
        int steps = mm*ration;
        stepper->move(steps, blocking);
        if (steps != 0) Stats::getInstance().onMotorSteps(motorName.c_str(), steps);
    }

    void moveMMAccum(float mm, float ration, bool blocking = false) {
        if (!ready()) return;
        finishContinuousRun(false);
        const float exact = (mm * ration) + Data::param.paperStepError;
        int32_t steps = 0;
        if (exact >= 0.0f) {
            steps = static_cast<int32_t>(floorf(exact + 1e-6f));
        } else {
            steps = static_cast<int32_t>(ceilf(exact - 1e-6f));
        }
        Data::param.paperStepError = exact - static_cast<float>(steps);
        stepper->move(steps, blocking);
        if (steps != 0) Stats::getInstance().onMotorSteps(motorName.c_str(), steps);
    }

    void forward2() {
        if (!ready()) return;
        stepper->forwardStep(false);
        Stats::getInstance().onMotorSteps(motorName.c_str(), 1);
    }

    void forward() {
        if (!ready()) return;
        stepper->forwardStep(true);
        delayMicroseconds(200);
        Stats::getInstance().onMotorSteps(motorName.c_str(), 1);
    }

    void backward() {
        if (!ready()) return;
        stepper->backwardStep(true);
        delayMicroseconds(200);
        Stats::getInstance().onMotorSteps(motorName.c_str(), 1);
    }

    void runForward() {
        if (!ready()) return;
        startContinuousRun();
        stepper->runForward();
    }

    void runBackward() {
        if (!ready()) return;
        startContinuousRun();
        stepper->runBackward();
    }

    void stopMove() {
        if (!ready()) return;
        stepper->stopMove();
        finishContinuousRun(true);
    }

    void forceStop() {
        if (!ready()) return;
        // FastAccelStepper::forceStop() sets an "immediate stop" latch in the
        // ramp generator. If forceStop() is called while the ramp generator is
        // idle (e.g. motor already stopped), that latch survives and the next
        // runForward/runBackward gets consumed just to clear it.
        //
        // Symptom: after pressing STOP (or right after boot) the motor starts
        // only on the second button press.
        //
        // So we must avoid calling FastAccelStepper::forceStop() when the ramp
        // generator is idle.
        if (stepper->isRampGeneratorActive()) {
            stepper->forceStop();
        } else if (stepper->isQueueRunning() || !stepper->isQueueEmpty()) {
            // Queue still has pulses, but ramp generator is already idle: stop
            // queue immediately without latching immediate-stop.
            stepper->forceStopAndNewPosition(stepper->getCurrentPosition());
        } else {
            // Fully idle: do nothing.
        }
        finishContinuousRun(false);
    }

    void forceStopAndNewPosition(int pos) {
        if (!ready()) return;
        stepper->forceStopAndNewPosition(pos);
        finishContinuousRun(false);
    }

    bool isRunning() { return ready() ? stepper->isRunning() : false; }
    bool isEnableDriver() { return enableDriver; }

    void ENA(bool enable) {
        if (!ready()) {
            enableDriver = false;
            return;
        }
        enableDriver = enable;
        if (enableDriver) {
            stepper->setEnablePin(enaPin, true);
            stepper->setAutoEnable(true);
            stepper->setDelayToEnable(2000);
            stepper->setDelayToDisable(6000);
        } else {
            stepper->setEnablePin(enaPin, false);
            stepper->setAutoEnable(false);
        }
    }


    void setCurrentPosition(int32_t new_pos) {
        if (!ready()) return;
        stepper->setCurrentPosition(new_pos);
    }

    int32_t getCurrentPosition(bool stop = false) {
        if (!ready()) return 0;
        return stop ? stepper->getCurrentPosition() + stepper->stepsToStop() : stepper->getCurrentPosition();
    }

    // Вызывается из ISR датчика — запоминает текущую позицию мотора в момент события.
    void IRAM_ATTR capturePosition() { _capturedPos = stepper->getCurrentPosition(); }

    // Позиция, сохранённая последним вызовом capturePosition().
    int32_t getCapturedPosition() const { return _capturedPos; }

    int32_t stepsToStop() { return ready() ? stepper->stepsToStop() : 0; }

    void loadParams(JsonObjectConst configObj) {
        _param.load(configObj);
    }

    uint32_t getMicroStep() { return static_cast<uint32_t>(_param.getValue("MicroStep", 0)); }
    int32_t getWorkMove() const { return _param.getValue("WorkMove", 0); }

    void setSpeed(Catalog::SPEED speed = Catalog::SPEED::Normal) {
        setSpeed(IStepperParam::speedModeName(speed));
    }

    void setSpeed(const std::string& mode) {
        if (!ready()) return;

        const int32_t speed = _param.getValue(mode, "Speed", 0);
        const int32_t acceleration = _param.getValue(mode, "Acceleration", 0);

        //Log::D("Speed: %d, Accel: %d", speed, acceleration);

        int32_t targetSpeed = speed;
        int32_t targetAcceleration = acceleration;
        if (targetSpeed < 0) targetSpeed = 0;
        if (targetAcceleration < 0) targetAcceleration = 0;

        stepper->setSpeedInHz(targetSpeed);
        stepper->setAcceleration(targetAcceleration);
    }

    int32_t getCheck(const std::string& mode, int32_t defaultValue = 0) const { return _param.getCheck(mode, defaultValue); }
    int32_t getCheckMs(const std::string& mode, int32_t defaultValue = -1) const { return _param.getCheckMs(mode, defaultValue); }

};
