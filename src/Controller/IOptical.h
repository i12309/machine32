#pragma once
#include <Arduino.h>
#include "IStepper.h"

class IEncoder;

class IOptical {
    int pin;
    int sig;
    int triggerState = -1;
    int32_t offsetSteps = 0;
    IStepper* mPaper;
    IEncoder* ePaper = nullptr;
    bool interruptAttached = false;

    TaskHandle_t taskHandle = nullptr;
    static void IRAM_ATTR sensor_isr(void* arg);
public:
    IOptical(int _pin, int _sig, IStepper* motor)
        : pin(_pin)
        , sig(_sig)
        , mPaper(motor)
    {
        pinMode(_pin, INPUT);
    }

    bool check(int sig){// Считываем состояние датчика
        return digitalRead(pin)==sig 
        ? true 
        : false; 
    }

    // Чтение значения с датчика
    int read() { return digitalRead(pin); }

    // Проверка на черную метку
    bool checkBlack() {
        return check(sig);
    }

    // Проверка на белую метку
    bool checkWhite() {
        return check(!sig);
    }

    void initTrigger();
    static void taskTrigger(void* pvParameters);
    // RISING -> ждать black, FALLING -> ждать white; реальный фронт вычисляется через sig датчика.
    void enableTrigger(int state, int32_t _offsetSteps = 0);
    void disableTrigger();

};
