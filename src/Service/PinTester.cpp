#include "PinTester.h"
#include "PinList.h"
#include "Controller/Registry.h"
#include "Controller/I2CBus.h"
#include "Log.h"

void PinTester::initTestMode() {
    Log::L("Инициализация режима тестирования пинов.");

    // --- Инициализация пинов ESP32 ---
    // Выходы моторов
    pinMode(MOTOR_STOL_STEP_PIN, OUTPUT);
    pinMode(MOTOR_PROTYAZHKA_STEP_PIN, OUTPUT);
    pinMode(MOTOR_GILOTINA_STEP_PIN, OUTPUT);
    pinMode(MOTOR_VIBROS_STEP_PIN, OUTPUT);
    pinMode(MOTOR_NOZH_1_STEP_PIN, OUTPUT);
    pinMode(MOTOR_NOZH_2_STEP_PIN, OUTPUT);
    pinMode(MOTOR_NOZH_3_STEP_PIN, OUTPUT);
    pinMode(MOTOR_NOZH_4_STEP_PIN, OUTPUT);
    pinMode(MOTOR_9_STEP_PIN, OUTPUT);
    pinMode(MOTOR_10_STEP_PIN, OUTPUT);
    pinMode(MOTOR_11_STEP_PIN, OUTPUT);
    pinMode(MOTOR_12_STEP_PIN, OUTPUT);
    pinMode(MOTOR_13_STEP_PIN, OUTPUT);
    pinMode(MOTOR_14_STEP_PIN, OUTPUT);
    
    // Входы датчиков (тоже переводим в OUTPUT для теста)
    pinMode(SENSOR_P2_PIN, OUTPUT);
    // pinMode(SENSOR_P35_PIN, OUTPUT); // GPIO35 can only be an input
    // pinMode(ANALOG_LINE_PIN, OUTPUT); // GPIO36 can only be an input
    // pinMode(MCP3_INTA_PIN, OUTPUT); // GPIO39 can only be an input
    // pinMode(MCP3_INTB_PIN, OUTPUT); // GPIO34 can only be an input

    // --- Инициализация пинов MCP ---
    // Имена MCP предполагаются "MCP0", "MCP1", "MCP2"
    for (int i = 0; i < 3; i++) {
        String mcpName = "MCP" + String(i);
        MCP* mcp = Registry::getInstance().getMCP(mcpName);
        if (mcp) {
            Log::L("Перевод всех пинов '%s' в режим OUTPUT.", mcpName.c_str());
            if (!I2CBus::lock(pdMS_TO_TICKS(20))) continue;
            for (int pin = 0; pin < 16; pin++) {
                mcp->pinMode(pin, OUTPUT);
            }
            I2CBus::unlock();
        } else {
            Log::L("MCP с именем '%s' не найден в реестре.", mcpName.c_str());
        }
    }
}

bool PinTester::setPinState(const String& device, int pin, int state) {
    if (state != HIGH && state != LOW) {
        Log::L("Ошибка: Неверное состояние пина (%d). Допустимо HIGH или LOW.", state);
        return false;
    }

    if (device == "ESP32") {
        if (pin < 0 || pin > 39) {
            Log::L("Ошибка: Неверный номер пина для ESP32 (%d).", pin);
            return false;
        }
        digitalWrite(pin, state);
        Log::L("Пин ESP32 %d установлен в состояние %s.", pin, state == HIGH ? "HIGH" : "LOW");
        return true;
    } 
    
    if (device.startsWith("MCP")) {
        if (pin < 0 || pin > 15) {
            Log::L("Ошибка: Неверный номер пина для MCP (%d).", pin);
            return false;
        }
        MCP* mcp = Registry::getInstance().getMCP(device);
        if (mcp) {
            if (!I2CBus::lock(pdMS_TO_TICKS(20))) return false;
            bool ok = mcp->digitalWrite(pin, state);
            I2CBus::unlock();
            if (!ok) return false;
            Log::L("Пин %d на '%s' установлен в состояние %s.", pin, device.c_str(), state == HIGH ? "HIGH" : "LOW");
            return true;
        } else {
            Log::L("Ошибка: MCP с именем '%s' не найден.", device.c_str());
            return false;
        }
    }

    Log::L("Ошибка: Неизвестное устройство '%s'.", device.c_str());
    return false;
}

int PinTester::getPinState(const String& device, int pin) {
    if (device == "ESP32") {
        if (pin < 0 || pin > 39) {
            Log::L("Ошибка: Неверный номер пина для ESP32 (%d).", pin);
            return -1;
        }
        return digitalRead(pin);
    }

    if (device.startsWith("MCP")) {
        if (pin < 0 || pin > 15) {
            Log::L("Ошибка: Неверный номер пина для MCP (%d).", pin);
            return -1;
        }
        MCP* mcp = Registry::getInstance().getMCP(device);
        if (mcp) {
            if (!I2CBus::lock(pdMS_TO_TICKS(20))) return -1;
            int v = mcp->digitalRead(pin, -1);
            I2CBus::unlock();
            return v;
        } else {
            Log::L("Ошибка: MCP с именем '%s' не найден.", device.c_str());
            return -1;
        }
    }

    Log::L("Ошибка: Неизвестное устройство '%s'.", device.c_str());
    return -1;
}
