#pragma once
#include "State/State.h"
#include "UI/Main/pINFO.h"

class Pressure : public State {

    int PIN_AO = 35; // ADC1_7 на ESP32
    int lastAdc = -1; 
    int lastMv  = -1; 

    public:
    Pressure() : State(State::Type::PRESSURE) {}

    void oneRun() override {
        analogReadResolution(12);                 // 0..4095
        analogSetPinAttenuation(PIN_AO, ADC_11db); // до ~3.3V 
    }

    State* run() override {
        // усреднение, чтобы меньше шумело
        uint32_t sum_adc = 0;
        uint32_t sum_mv = 0;
        for (int i = 0; i < 20; i++) {
            sum_adc += analogRead(PIN_AO);
            sum_mv += analogReadMilliVolts(PIN_AO);
            delay(2);
        }
        int adc = sum_adc / 20;
        int mv = sum_mv / 20;

        Log::D("ADC = %d, mV = %d", adc, mv);

        if (Page::activePage == &pINFO::getInstance()) { // если страница активна
            if (adc != lastAdc) { pINFO::getInstance().tInfo1.setText(String(adc).c_str()); lastAdc = adc; }
            if (mv  != lastMv)  { pINFO::getInstance().tInfo2.setText(String(mv).c_str());  lastMv  = mv;  }
        }

        delay(1000);
        return this;
    }
};