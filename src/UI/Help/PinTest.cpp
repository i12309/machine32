#include "PinTest.h"
#include "Service/PinTester.h"
#include "Service/PinList.h"

void PinTest::pop_bBack(void* ptr)
{
    Log::D(__func__);
    ESP.restart();
}

void PinTest::pop_radio(void* ptr) 
{
    Log::D(__func__);
    PinTest& UI = PinTest::getInstance();

    // Reset all physical pins to LOW
    for (int i = 0; i < sizeof(espPins) / sizeof(espPins[0]); i++) PinTester::setPinState("ESP32", espPins[i], 0);
    for (int i = 0; i < sizeof(mcp1Pins) / sizeof(mcp1Pins[0]); i++) PinTester::setPinState("MCP0", mcp1Pins[i], 0);
    for (int i = 0; i < sizeof(mcp2Pins) / sizeof(mcp2Pins[0]); i++) PinTester::setPinState("MCP1", mcp2Pins[i], 0);
    for (int i = 0; i < sizeof(mcp3Pins) / sizeof(mcp3Pins[0]); i++) PinTester::setPinState("MCP2", mcp3Pins[i], 0);
    
    const int* pins = nullptr;
    int num_pins = 0;

    UI.esp.setValue(0);
    UI.mcp1.setValue(0);
    UI.mcp2.setValue(0);
    UI.mcp3.setValue(0);

    if (ptr == &UI.esp) {
        UI.currentDevice = PinTest::Device::DEV_ESP32;
        Log::D("Selected device: ESP32");
        UI.esp.setValue(1);
        pins = espPins;
        num_pins = sizeof(espPins) / sizeof(espPins[0]);
    } else if (ptr == &UI.mcp1) {
        UI.currentDevice = PinTest::Device::DEV_MCP1;
        Log::D("Selected device: MCP1");
        UI.mcp1.setValue(1);
        pins = mcp1Pins;
        num_pins = sizeof(mcp1Pins) / sizeof(mcp1Pins[0]);
    } else if (ptr == &UI.mcp2) {
        UI.currentDevice = PinTest::Device::DEV_MCP2;
        Log::D("Selected device: MCP2");
        UI.mcp2.setValue(1);
        pins = mcp2Pins;
        num_pins = sizeof(mcp2Pins) / sizeof(mcp2Pins[0]);
    } else if (ptr == &UI.mcp3) {
        UI.currentDevice = PinTest::Device::DEV_MCP3;
        Log::D("Selected device: MCP3");
        UI.mcp3.setValue(1);
        pins = mcp3Pins;
        num_pins = sizeof(mcp3Pins) / sizeof(mcp3Pins[0]);
    }

    // Сброс всех переключателей в состояние LOW и обновление интерфейса
    for (int i = 0; i < 21; i++) {
        UI.Switch[i]->setValue(0);
        bool pin_is_used = (i < num_pins);

        if (pin_is_used) {
            // Показать элемент и обновить текст
            char cmd[30];
            if (i > 14) { // 3-я строка
                sprintf(cmd, "p%d.y=201", i + 1); sendCommand(cmd);
                sprintf(cmd, "t%d.y=223", i + 1); sendCommand(cmd);
            }
            char buf[10];
            sprintf(buf, "%d", pins[i]);
            UI.Text[i]->setText(buf);
        } else {
            // Скрыть элемент
            char cmd[30];
            sprintf(cmd, "p%d.y=1000", i + 1); sendCommand(cmd);
            sprintf(cmd, "t%d.y=1000", i + 1); sendCommand(cmd);
        }
    }
}

void PinTest::pop_switch(void* ptr)
{
    Log::D(__func__);
    PinTest& UI = PinTest::getInstance();
    
    if (UI.currentDevice == PinTest::Device::DEV_NONE) {
        Log::D("No device selected!");
        ((NexDSButton*)ptr)->setValue(0);
        return;
    }

    int switch_index = -1;
    for (int i = 0; i < 21; i++) {
        if (ptr == UI.Switch[i]) {
            switch_index = i;
            break;
        }
    }

    if (switch_index == -1) return;

    uint32_t state = 0;
    UI.Switch[switch_index]->getValue(&state);

    String deviceName = "";
    int pin = -1;

    switch (UI.currentDevice) {
        case DEV_ESP32:
            if (switch_index < sizeof(espPins) / sizeof(espPins[0])) {
                deviceName = "ESP32";
                pin = espPins[switch_index];
            }
            break;
        case DEV_MCP1:
            if (switch_index < sizeof(mcp1Pins) / sizeof(mcp1Pins[0])) {
                deviceName = "MCP0";
                pin = mcp1Pins[switch_index];
            }
            break;
        case DEV_MCP2:
            if (switch_index < sizeof(mcp2Pins) / sizeof(mcp2Pins[0])) {
                deviceName = "MCP1";
                pin = mcp2Pins[switch_index];
            }
            break;
        case DEV_MCP3:
            if (switch_index < sizeof(mcp3Pins) / sizeof(mcp3Pins[0])) {
                deviceName = "MCP2";
                pin = mcp3Pins[switch_index];
            }
            break;
        default:
            break;
    }

    if (pin != -1) {
        Log::D("Setting pin %d on %s to %s", pin, deviceName.c_str(), state == 1 ? "HIGH" : "LOW");
        PinTester::setPinState(deviceName, pin, state);
    } else {
        Log::D("Pin index %d is out of bounds for the selected device.", switch_index);
        UI.Switch[switch_index]->setValue(0);
    }
}
