#pragma once
#include <FastAccelStepper.h>
#include <array>
#include <map>

#include "Core.h"
#include "MCP.h"
#include "IStepper.h"
#include "ISensor.h"
#include "IOptical.h"
#include "IClutch.h"
#include "ISwitch.h"
#include "IButton.h"
#include "IEncoder.h"
#include "I2CBus.h"

class Registry {

    std::map<String, IStepper*> motors;
    std::map<String, ISensor*> sensors;
    std::map<String, IOptical*> opticals;
    std::map<String, IClutch*> clutchs;
    std::map<String, ISwitch*> switchs;
    std::map<String, IButton*> buttons;
    std::map<String, IEncoder*> encoders;

    // Множественные MCP23017
    std::map<String, MCP*> MCPs;
    std::array<MCP*, 16> mcpByNumber{};

    FastAccelStepperEngine engine;

    // точка входа в config где список устройств для инициализации
    JsonObjectConst deviceObj;
    
public:

    Registry() : 
        engine(FastAccelStepperEngine())
    {
        // Инициализация движка FastAccelStepper
        engine.init();
    };

    static Registry& getInstance() { static Registry instance; return instance; }

    // моторы 
    void registerMotor(const String& name, IStepper* motor) {motors[name] = motor;}
    IStepper* getMotor(const String& name) const {
        auto it = motors.find(name);
        if (it == motors.end()) return nullptr;
        return it->second;
    }
    const std::map<String, IStepper*>& getMotors() const { return motors; }
    void emergencyMotor() { for (const auto& pair : motors) if (pair.second != nullptr) pair.second->forceStop(); }

    //сенсоры
    void registerSensor(const String& name, ISensor* sensor) {sensors[name] = sensor;}
    ISensor* getSensor(const String& name) const {
        auto it = sensors.find(name);
        if (it == sensors.end()) return nullptr;
        return it->second;
    }

    // оптические датчики
    void registerOptical(const String& name, IOptical* optical) {opticals[name] = optical;}
    IOptical* getOptical(const String& name) const {
        auto it = opticals.find(name);
        if (it == opticals.end()) return nullptr;
        return it->second;
    }

    // муфты
    void registerClutch(const String& name, IClutch* clutch) {clutchs[name] = clutch;}
    IClutch* getClutch(const String& name) const {
        auto it = clutchs.find(name);
        if (it == clutchs.end()) return nullptr;
        return it->second;
    }
    void emergencyClutch() { for (const auto& pair : clutchs) if (pair.second != nullptr) pair.second->disengage(); }

    // Переключатели
    void registerSwitch(const String& name, ISwitch* sw) {switchs[name] = sw;}
    ISwitch* getSwitch(const String& name) const {
        auto it = switchs.find(name);
        if (it == switchs.end()) return nullptr;
        return it->second;
    }
    void emergencySwitch() { for (const auto& pair : switchs) if (pair.second != nullptr) pair.second->off(); }

    // Кнопки
    void registerButton(const String& name, IButton* button) {buttons[name] = button;}
    IButton* getButton(const String& name) const {
        auto it = buttons.find(name);
        if (it == buttons.end()) return nullptr;
        return it->second;
    }

    // Энкодеры
    void registerEncoder(const String& name, IEncoder* encoder) {encoders[name] = encoder;}
    IEncoder* getEncoder(const String& name) const {
        auto it = encoders.find(name);
        if (it == encoders.end()) return nullptr;
        return it->second;
    }

    // MCP
    void registerMCP(const String& name, MCP* mcp) {
        MCPs[name] = mcp;
        if (name.length() > 3 && name.startsWith("MCP")) {
            int idx = name.substring(3).toInt();
            if (idx >= 0 && idx < static_cast<int>(mcpByNumber.size())) {
                mcpByNumber[static_cast<size_t>(idx)] = mcp;
            }
        }
    }
    MCP* getMCP(const String& name) const {
        auto it = MCPs.find(name);
        if (it == MCPs.end()) return nullptr;
        return it->second;
    }
    MCP* getMCP(uint8_t number) const {
        if (number >= mcpByNumber.size()) return nullptr;
        return mcpByNumber[number];
    }

    // Инициализация из конфига
    bool init(String* error_message) {
        Log::D("init - Чтение переферии");

        deviceObj = Core::config.doc["device"];
        if (deviceObj.isNull()) {
            if (error_message) *error_message = "Секция 'device' не найдена или некорректна.";
            return false;
        }

        // Инициализация MCP23017 устройств из конфига
        if (initMCPs(error_message) == false ) return false;

        // Динамическое создание моторов
        if (initMotors(error_message) == false ) return false;

        // Динамическое создание муфт (clutchs)
        if (initClutchs(error_message) == false ) return false;

        // Динамическое создание переключателей (switchs)
        if (initSwitchs(error_message) == false ) return false;

        // Динамическое создание сенсоров
        if (initSensors(error_message) == false ) return false;

        // Динамическое создание оптических датчиков
        if (initOpticals(error_message) == false ) return false;

        // Динамическое создание кнопок
        if (initButtons(error_message) == false ) return false;

        // Динамическое создание энкодеров
        if (initEncoders(error_message) == false ) return false;

        return true;
    }

    // Инициализация MCP23017 устройств из конфига
    bool initMCPs(String* error_message) {
        JsonObjectConst i2cObj = deviceObj["I2C"];
        if (i2cObj.isNull()) {
            if (error_message) *error_message = "Секция 'I2C' не найдена.";
            return false;
        }

        // Устанавливаем колбэк для внешних пинов (DIR/EN) через MCP23017
        if (!I2CBus::init()) {
            if (error_message) *error_message = "I2C init failed.";
            return false;
        }

        engine.setExternalCallForPin(ExternalPinCallback);

        for (JsonPairConst i2cPair : i2cObj) {

            const String& name = i2cPair.key().c_str();
            JsonObjectConst i2cConfig = i2cPair.value();
            
            String type = i2cConfig["type"] | "";
            String addressStr = i2cConfig["address"] | "";
            int intA = i2cConfig["intA"] | -1;
            int intB = i2cConfig["intB"] | -1;

            if (!type.isEmpty() && !addressStr.isEmpty()) {
                Log::L("[Registry] MCP config name=%s addr=%s intA=%d intB=%d",
                    name.c_str(),
                    addressStr.c_str(),
                    intA,
                    intB
                );
                MCP* newMCP = nullptr;
                const uint8_t addr = static_cast<uint8_t>(strtol(addressStr.c_str(), nullptr, 16));
                bool present = false;
                if (I2CBus::lock(pdMS_TO_TICKS(20))) {
                    Wire.beginTransmission(addr);
                    present = (Wire.endTransmission(true) == 0);
                    I2CBus::unlock();
                }

                if (present) {
                    newMCP = new MCP(name, type, addressStr, intA, intB);
                } else {
                    Log::D("MCP23017 '%s' нет на I2C шине.", name.c_str());
                }

                registerMCP(name, newMCP);
                if (newMCP != nullptr && newMCP->IO() != nullptr) Log::D("MCP23017 '%s' создан и зарегистрирован (online).", name.c_str());
                else Log::D("MCP23017 '%s' создан и зарегистрирован (offline).", name.c_str());
                delay(1); // Даем планировщику время во время обнаружения периферии, чтобы не словить watchdog.
            } else {
                if (error_message) *error_message = "Ошибка: MCP23017 '" + name + "'.";
                return false;
            }
        }

        return true;
    }
    
    bool initMotors(String* error_message) {
        
        JsonObjectConst motorsObj = deviceObj["motors"];
        if (!motorsObj.isNull()) {
            for (JsonPairConst motorPair : motorsObj) {
                const String& name = motorPair.key().c_str();
                JsonObjectConst motorConfig = motorPair.value();

                String i2c_name = motorConfig["I2C"] | "ESP32";
                JsonObjectConst pinObj = motorConfig["pin"];
                int step_pin = pinObj["step"] | (motorConfig["step_pin"] | -1);
                int dir_pin = pinObj["dir"] | (motorConfig["dir_pin"] | -1);
                int ena_pin = pinObj["ena"] | (motorConfig["ena_pin"] | -1);

                if (step_pin != -1 && dir_pin != -1 && ena_pin != -1) {

                    // Проверяем что MCP23017 существует
                    MCP* mcp = (i2c_name != "ESP32") ? getMCP(i2c_name) : nullptr;
                    bool enabled = true;
                    if (i2c_name != "ESP32" && mcp == nullptr) {
                        enabled = false;
                    }
                    if (i2c_name != "ESP32" && (mcp == nullptr || mcp->IO() == nullptr)) {
                        enabled = false;
                        Log::L("[Registry] Motor '%s' disabled: MCP '%s' offline", name.c_str(), i2c_name.c_str());
                    }
                    IStepper* newMotor = new IStepper(step_pin, dir_pin, ena_pin, engine, mcp, enabled);
                    registerMotor(name, newMotor);
                    newMotor->loadParams(motorConfig);
                    newMotor->setMotorName(name.c_str());
                    Log::D("Мотор '%s' создан и зарегистрирован.", name.c_str());
                } else {
                    if (error_message) *error_message = "Ошибка: Неполные пины для мотора '" + name + "'.";
                    return false;
                }
            }
        } else {
            if (error_message) *error_message = "Ошибка: секция [motors] не найдена";
            return false;
        }

        return true;
    }

    bool initClutchs(String* error_message) {
        JsonObjectConst clutchsObj = deviceObj["clutchs"];
        if (!clutchsObj.isNull()) {
            for (JsonPairConst clutchPair : clutchsObj) {
                const String& name = clutchPair.key().c_str();
                JsonObjectConst clutchConfig = clutchPair.value();

                String i2c_name = clutchConfig["I2C"] | "ESP32";
                int pin = clutchConfig["pin"] | -1;

                if (pin != -1) {
                    // Проверяем что MCP23017 существует
                    MCP* mcp = (i2c_name != "ESP32") ? getMCP(i2c_name) : nullptr;
                    bool enabled = true;
                    if (i2c_name != "ESP32" && mcp == nullptr) {
                        enabled = false;
                    }
                    if (i2c_name != "ESP32" && (mcp == nullptr || mcp->IO() == nullptr)) {
                        enabled = false;
                        Log::L("[Registry] Clutch '%s' disabled: MCP '%s' offline", name.c_str(), i2c_name.c_str());
                    }
                    IClutch* newClutch = new IClutch(pin, mcp, enabled);
                    registerClutch(name, newClutch);
                    Log::D("Муфта '%s' создана и зарегистрирована.", name.c_str());
                } else {
                    if (error_message) *error_message = "Ошибка: Неполные пины для муфты '" + name + "'.";
                    return false;
                }
            }
        } else {
            // Legacy секция: может отсутствовать, если станок переведен на switchs.
            return true;
        }
        return true;
    }

    bool initSwitchs(String* error_message) {
        JsonObjectConst switchsObj = deviceObj["switchs"];
        if (!switchsObj.isNull()) {
            for (JsonPairConst switchPair : switchsObj) {
                const String& name = switchPair.key().c_str();
                JsonObjectConst switchConfig = switchPair.value();

                String i2c_name = switchConfig["I2C"] | "ESP32";
                int pin = switchConfig["pin"] | -1;
                int sig = switchConfig["sig"] | HIGH;

                if (pin != -1 && (sig == LOW || sig == HIGH)) {
                    MCP* mcp = (i2c_name != "ESP32") ? getMCP(i2c_name) : nullptr;
                    bool enabled = true;
                    if (i2c_name != "ESP32" && mcp == nullptr) {
                        enabled = false;
                    }
                    if (i2c_name != "ESP32" && (mcp == nullptr || mcp->IO() == nullptr)) {
                        enabled = false;
                        Log::L("[Registry] Switch '%s' disabled: MCP '%s' offline", name.c_str(), i2c_name.c_str());
                    }
                    ISwitch* newSwitch = new ISwitch(pin, sig, mcp, enabled);
                    registerSwitch(name, newSwitch);
                    Log::D("Переключатель '%s' создан и зарегистрирован.", name.c_str());
                } else {
                    if (error_message) *error_message = "Ошибка: Неполные параметры для switch '" + name + "'.";
                    return false;
                }
            }
        } else {
            if (error_message) *error_message = "Ошибка: секция [switchs] не найдена";
            return false;
        }
        return true;
    }

    bool initSensors(String* error_message) {
        JsonObjectConst sensorsObj = deviceObj["sensors"];
        if (!sensorsObj.isNull()) {
            for (JsonPairConst sensorPair : sensorsObj) {
                const String& name = sensorPair.key().c_str();
                JsonObjectConst sensorConfig = sensorPair.value();

                String i2c_name = sensorConfig["I2C"] | "ESP32";
                int pin = sensorConfig["pin"] | -1;
                int typePin = INPUT_PULLUP;

                if (pin != -1) {
                    // Проверяем что MCP23017 существует
                    MCP* mcp = (i2c_name != "ESP32") ? getMCP(i2c_name) : nullptr;
                    bool enabled = true;
                    if (i2c_name != "ESP32" && mcp == nullptr) {
                        enabled = false;
                    }
                    if (i2c_name != "ESP32" && (mcp == nullptr || mcp->IO() == nullptr)) {
                        enabled = false;
                        Log::L("[Registry] Sensor '%s' disabled: MCP '%s' offline", name.c_str(), i2c_name.c_str());
                    }
                    ISensor* newSensor = new ISensor(pin, typePin, mcp, enabled);
                    registerSensor(name, newSensor);
                    
                    // Триггеры MCP во время работы управляются через McpTrigger.
                    Log::D("Сенсор '%s' создан и зарегистрирован. Пин: %d", name.c_str(), pin);
                } else {
                    if (error_message) *error_message = "Ошибка: Неполные пины для сенсора '" + name + "'.";
                    return false;
                }
            }
        } else {
            if (error_message) *error_message = "Ошибка: секция [sensors] не найдена";
            return false;
        }
        return true;
    }

    bool initOpticals(String* error_message) {
        JsonObjectConst opticalsObj = deviceObj["optical"];
        if (!opticalsObj.isNull()) {
            for (JsonPairConst opticalPair : opticalsObj) {
                const String& name = opticalPair.key().c_str();
                JsonObjectConst opticalConfig = opticalPair.value();

                int pin = opticalConfig["pin"] | -1;
                int sig = opticalConfig["sig"] | 1;

                if (pin != -1 && (sig == 0 || sig == 1)) {
                    IOptical* newOptical = new IOptical(pin, sig, getMotor("PAPER"));
                    registerOptical(name, newOptical);
                    newOptical->initTrigger();
                    Log::D("Оптический датчик '%s' создан и зарегистрирован. Пин: %d", name.c_str(), pin);
                } else {
                    if (error_message) *error_message = "Ошибка: Неполные пины для оптического датчика '" + name + "'.";
                    return false;
                }
            }
        } else {
            if (error_message) *error_message = "Ошибка: секция [optical] не найдена";
            return false;
        }
        return true;
    }

    bool initButtons(String* error_message) {
        JsonObjectConst buttonsObj = deviceObj["buttons"];
        if (!buttonsObj.isNull()) {
            for (JsonPairConst buttonPair : buttonsObj) {
                const String& name = buttonPair.key().c_str();
                JsonObjectConst buttonConfig = buttonPair.value();

                String i2c_name = buttonConfig["I2C"] | "ESP32";
                int pin = buttonConfig["pin"] | -1;
                int typePin = INPUT;
                int sig = HIGH;

                if (pin != -1) {
                    // Проверяем что MCP23017 существует
                    MCP* mcp = (i2c_name != "ESP32") ? getMCP(i2c_name) : nullptr;
                    bool enabled = true;
                    if (i2c_name != "ESP32" && mcp == nullptr) {
                        enabled = false;
                    }
                    if (i2c_name != "ESP32" && (mcp == nullptr || mcp->IO() == nullptr)) {
                        enabled = false;
                        Log::L("[Registry] Button '%s' disabled: MCP '%s' offline", name.c_str(), i2c_name.c_str());
                    }
                    IButton* newButton = new IButton(0, pin, typePin, sig, mcp, enabled);
                    registerButton(name, newButton);
                    Log::D("Кнопка '%s' создана и зарегистрирована.", name.c_str());

                } else {
                    if (error_message) *error_message = "Ошибка: Неполные пины для кнопки '" + name + "'.";
                    return false;
                }
            }
        } else {
            if (error_message) *error_message = "Ошибка: секция [buttons] не найдена";
            return false;
        }
        return true;
    }

    bool initEncoders(String* error_message) {
        JsonObjectConst encodersObj = deviceObj["encoders"];
        if (!encodersObj.isNull()) {
            for (JsonPairConst encoderPair : encodersObj) {
                const String& name = encoderPair.key().c_str();
                JsonObjectConst encoderConfig = encoderPair.value();

                String i2c_name = encoderConfig["I2C"] | "ESP32";
                int pinA = encoderConfig["pinA"] | -1;
                int pinB = encoderConfig["pinB"] | -1;
                int pcnt = encoderConfig["pcnt"] | 6;
                int threshold = encoderConfig["threshold"] | -1;

                if (pinA != -1 && pinB != -1) {
                    bool enabled = true;
                    IEncoder* newEncoder = new IEncoder(pinA, pinB, threshold, nullptr, enabled, pcnt);
                    registerEncoder(name, newEncoder);
                    Log::D("Энкодер '%s' создан и зарегистрирован. Пины: %d, %d, PCNT: %d", name.c_str(), pinA, pinB, pcnt);
                } else {
                    if (error_message) *error_message = "Ошибка: Неполные пины для энкодера '" + name + "'.";
                    return false;
                }
            }
        } else {
            if (error_message) *error_message = "Ошибка: секция [encoders] не найдена";
            return false;
        }
        return true;
    }

    void reset() {
        //print();
        Log::D("Обновляем параметры для переферии");

        for (const auto& pair : motors) {
            const String& name = pair.first;
            IStepper* stepper = pair.second;
            if (pair.second !=nullptr){
                stepper->setSpeed(); // Перезагружаем параметры
                stepper->setCurrentPosition(0); // Сбрасываем позицию
            }
        }
    }

    void print() {
        Log::D("Список моторов:");
        for (const auto& pair : motors) Log::D("- %s:%s", pair.first.c_str(), (pair.second==nullptr)?"null":"object");
        Log::D("Список сенсоров:");
        for (const auto& pair : sensors) Log::D("- %s:%s", pair.first.c_str(), (pair.second==nullptr)?"null":"object");
        Log::D("Список оптика:");
        for (const auto& pair : opticals) Log::D("- %s:%s", pair.first.c_str(), (pair.second==nullptr)?"null":"object");
        Log::D("Список муфты:");
        for (const auto& pair : clutchs) Log::D("- %s:%s", pair.first.c_str(), (pair.second==nullptr)?"null":"object");
        Log::D("Список переключателей:");
        for (const auto& pair : switchs) Log::D("- %s:%s", pair.first.c_str(), (pair.second==nullptr)?"null":"object");
        Log::D("Список кнопки:");
        for (const auto& pair : buttons) Log::D("- %s:%s", pair.first.c_str(), (pair.second==nullptr)?"null":"object");
        Log::D("Список энкодеров:");
        for (const auto& pair : encoders) Log::D("- %s:%s", pair.first.c_str(), (pair.second==nullptr)?"null":"object");
        Log::D("Список MCP:");
        for (const auto& pair : MCPs) Log::D("- %s:%s", pair.first.c_str(), (pair.second==nullptr)?"null":"object");
    }

    // Статический колбэк для FastAccelStepper
    static bool ExternalPinCallback(uint8_t pin, uint8_t value) {
        uint8_t pin_without_flag = pin & ~PIN_EXTERNAL_FLAG;  // Убираем флаг внешнего пина.
        uint8_t mcp_index = (pin_without_flag >> 4) & 0x0F;
        uint8_t mcp_pin = pin_without_flag & 0x0F;
        MCP* mcp = Registry::getInstance().getMCP(mcp_index);
        if (mcp == nullptr || mcp->IO() == nullptr) return !value;

        // Держим callback коротким и детерминированным: только запись, без readback.
        // Если шина занята, возвращаем противоположное состояние, чтобы FastAccelStepper повторил попытку.
        if (!I2CBus::tryLock()) return !value;
        if (!mcp->digitalWrite(mcp_pin, value)) {
            I2CBus::unlock();
            return !value;
        }
        I2CBus::unlock();
        return value;
    }

    static bool pinConnect(int pin, int mode, int sig = -1){
        Log::D("%s %d %d",__func__,pin,mode);
        int DELAY_BETWEEN_SAMPLES = 100;
        int SAMPLES = 10;

        if (mode == INPUT_PULLUP || mode == INPUT_PULLDOWN){
            if (sig == -1) sig = (mode == INPUT_PULLUP) ?LOW :HIGH;
            int Count = 0;
            // Проводим несколько измерений
            for (int i = 0; i < SAMPLES; i++) {
              if (digitalRead(pin) == sig) Count++; 
              delay(DELAY_BETWEEN_SAMPLES);  // Задержка между измерениями
            }
            // Анализируем результаты
            return (Count == SAMPLES);
        }

        if (mode == OUTPUT){
            // Сохраняем текущее состояние пина
            //bool previousState = digitalRead(pin);

            // Переключаем пин в режим INPUT
            pinMode(pin, INPUT);

            // Читаем состояние пина
            // Счетчики для HIGH и LOW
            int highCount = 0;
            int lowCount = 0;

            // Проводим 10 измерений
            for (int i = 0; i < SAMPLES; i++) {
                bool pinState = digitalRead(pin);
                if (pinState == HIGH) highCount++; else lowCount++;
                delay(DELAY_BETWEEN_SAMPLES);  // Задержка между измерениями
            }

            // Возвращаем пин в режим OUTPUT и восстанавливаем состояние
            pinMode(pin, OUTPUT);
            //digitalWrite(pin, previousState);
            
            // все 10 повторений в одном состоянии, значит подключение есть 
            return ((highCount == SAMPLES && lowCount == 0) || (highCount == 0 && lowCount == SAMPLES));
        }

        return false;
    }

};






