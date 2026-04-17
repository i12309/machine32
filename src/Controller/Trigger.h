#pragma once
#include <functional>
#include <map>
#include <vector>

#include "Registry.h"
#include "State/PlanManager.h"

enum ProcessType {
    SENSOR,
    ENCODER,
    OPTICAL,
    BUTTON,
    MOTOR_TIMEOUT  // Периодический вызов callback пока мотор запущен — для контроля таймаута через Scene
};

// Структура для хранения информации о триггере
struct TriggerAction {
    ProcessType typeProcess;
    std::string deviceName;
    std::string motorName;
    int expectedSignal;  // HIGH или LOW, или для optical 0/1 black/white
    std::function<void()> callback;  // Функция, которая выполнится при срабатывании
    bool isActive;
    uint32_t startTime;
    uint32_t timeout;  // Таймаут в миллисекундах
    int32_t maxSteps;  // Максимальное количество шагов
    int32_t startPosition;  // Начальная позиция мотора
};

class Trigger {
private:
    static Trigger* instance;
    std::map<std::string, TriggerAction> activeTriggers;
    Registry* registry;

public:
    static Trigger& getInstance() {if (!instance) instance = new Trigger();return *instance;}

    Trigger() : registry(&Registry::getInstance()) {}
    
    void registerTrigger();

    // Регистрация триггера для сенсора
    void registerSensorTrigger(const std::string& triggerName,
                        const std::string& sensorName,
                        const std::string& motorName,
                        int expectedSignal,
                        uint32_t timeout,
                        int32_t maxSteps,
                        std::function<void()> callback
                    ) {

        TriggerAction trigger;
        trigger.typeProcess = SENSOR;
        trigger.deviceName = sensorName;
        trigger.motorName = motorName;
        trigger.expectedSignal = expectedSignal;
        trigger.callback = callback;
        trigger.isActive = false;
        trigger.timeout = timeout;
        trigger.maxSteps = maxSteps;

        activeTriggers[triggerName] = trigger;
    }

    // Регистрация триггера для энкодера
    void registerEncoderTrigger(const std::string& triggerName,
                        const std::string& encoderName,
                        std::function<void()> callback
                    ) {

        TriggerAction trigger;
        trigger.typeProcess = ENCODER;
        trigger.deviceName = encoderName;
        trigger.callback = callback;
        trigger.isActive = false;  // Энкодеры всегда активны
        trigger.timeout = 0;
        trigger.maxSteps = -1;

        activeTriggers[triggerName] = trigger;
    }

    // Регистрация триггера для оптического датчика
    void registerOpticalTrigger(const std::string& triggerName,
                        const std::string& opticalName,
                        int expectedSignal,  // 0 - black, 1 - white
                        std::function<void()> callback
                    ) {

        TriggerAction trigger;
        trigger.typeProcess = OPTICAL;
        trigger.deviceName = opticalName;
        trigger.expectedSignal = expectedSignal;
        trigger.callback = callback;
        trigger.isActive = false;
        trigger.timeout = 0;
        trigger.maxSteps = -1;

        activeTriggers[triggerName] = trigger;
    }

    // Регистрация триггера для кнопки
    void registerButtonTrigger(const std::string& triggerName,
                        const std::string& buttonName,
                        std::function<void()> callback
                    ) {

        TriggerAction trigger;
        trigger.typeProcess = BUTTON;
        trigger.deviceName = buttonName;
        trigger.callback = callback;
        trigger.isActive = false;
        trigger.timeout = 0;
        trigger.maxSteps = -1;

        activeTriggers[triggerName] = trigger;
    }

    static void init();
    // Активация триггера
    void activateTrigger(const std::string& triggerName) {
        auto it = activeTriggers.find(triggerName);
        if (it != activeTriggers.end()) {
            // Запоминаем начальную позицию мотора
            IStepper* motor = registry->getMotor(it->second.motorName.c_str());
            if (motor) {
                it->second.startPosition = motor->getCurrentPosition();
            }
            it->second.startTime = millis();
            it->second.isActive = true;
        }
    }

    // Деактивация триггера
    void deactivateTrigger(const std::string& triggerName) {
        auto it = activeTriggers.find(triggerName);
        if (it != activeTriggers.end()) {
            it->second.isActive = false;
        }
    }

    // Основная функция обработки - должна вызываться в главном цикле
    void process() {
        for (auto& pair : activeTriggers) {
            TriggerAction& trigger = pair.second;

            if (!trigger.isActive) continue;

            switch (trigger.typeProcess) {
                case SENSOR:
                    processSensor(trigger);
                    break;
                case ENCODER:
                    processEncoder(trigger);
                    break;
                case OPTICAL:
                    processOptical(trigger);
                    break;
                case BUTTON:
                    processButton(trigger);
                    break;
                case MOTOR_TIMEOUT:
                    processMotorTimeout(trigger);
                    break;
            }
        }
    }

    void processSensor(TriggerAction& trigger) {
        ISensor* sensor = registry->getSensor(trigger.deviceName.c_str());
        IStepper* motor = registry->getMotor(trigger.motorName.c_str());

        if (!sensor || !motor) return;

        bool shouldTrigger = false;
        uint32_t currentTime = millis();

        // Проверяем срабатывание датчика
        if (sensor->check(trigger.expectedSignal)) shouldTrigger = true;
        // Проверяем таймаут
        if (trigger.timeout > 0 && currentTime - trigger.startTime > trigger.timeout) shouldTrigger = true;
        // Проверяем максимальное количество шагов
        if (trigger.maxSteps > 0) {
            int32_t stepsDone = abs(motor->getCurrentPosition() - trigger.startPosition);
            //Log::D("stepsDone: %d, startPosition: %d, Curr: %d, maxSteps: %d",stepsDone,trigger.startPosition,motor->getCurrentPosition(),trigger.maxSteps);
            if (stepsDone >= trigger.maxSteps) shouldTrigger = true;
        }

        // Если триггер сработал
        if (shouldTrigger) {
            trigger.isActive = false;
            trigger.callback();
        }
    }

    void processEncoder(TriggerAction& trigger) {
        IEncoder* encoder = registry->getEncoder(trigger.deviceName.c_str());
        if (!encoder) return;
        if (encoder->getCount() > encoder->getThreshold()) trigger.callback();
    }

    void processOptical(TriggerAction& trigger) {
        IOptical* optical = registry->getOptical(trigger.deviceName.c_str());
        if (!optical) return;

        //if (trigger.expectedSignal == 0 && optical->check511()) trigger.callback();
        //else if (trigger.expectedSignal == 1 && optical->checkWhite()) {trigger.callback();}
    }

    // Обработчик таймаута мотора: вызывает callback каждый цикл process() пока триггер активен.
    // Не управляет остановкой напрямую — это делает callback (например Scene::tableStop(NotStop)).
    void processMotorTimeout(TriggerAction& trigger) {
        IStepper* motor = registry->getMotor(trigger.motorName.c_str());
        if (!motor) return;
        trigger.callback();
    }

    void processButton(TriggerAction& trigger) {
        IButton* button = registry->getButton(trigger.deviceName.c_str());
        if (!button) return;

        //if (button->isTrigger()) {trigger.callback();}
    }

    // Регистрация триггера периодической проверки таймаута мотора.
    // Триггер активен всегда (isActive = true), но callback вызывается только если мотор запущен.
    // Вся логика таймаута живёт в Scene::Timeout — здесь лишь обеспечивается периодический опрос.
    void registerMotorTimeoutTrigger(const std::string& triggerName,
                        const std::string& motorName,
                        std::function<void()> callback
                    ) {
        TriggerAction trigger;
        trigger.typeProcess = MOTOR_TIMEOUT;
        trigger.deviceName = "";
        trigger.motorName = motorName;
        trigger.callback = callback;
        trigger.isActive = false;  // Активируется вручную через activateTrigger()
        trigger.timeout = 0;
        trigger.maxSteps = -1;
        trigger.startTime = 0;
        trigger.startPosition = 0;

        activeTriggers[triggerName] = trigger;
    }

    // Проверка, активен ли триггер
    bool isTriggerActive(const std::string& triggerName) {
        auto it = activeTriggers.find(triggerName);
        return it != activeTriggers.end() && it->second.isActive;
    }

    // Очистка всех триггеров
    void clearAllTriggers() {
        activeTriggers.clear();
    }
};
