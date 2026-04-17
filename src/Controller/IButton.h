#pragma once

#include "Service/Log.h"
#include "MCP.h"

//#define BUTTONS 1

class IButton {
private:
    int _sensorPin;
    int _typePin; //INPUT 1, OUTPUT 3, PULLUP 4, INPUT_PULLUP 5, PULLDOWN 8, INPUT_PULLDOWN 9
    int _Sig; // LOW или HIGH 
    MCP* _mcp;
    bool _use_mcp;
    bool _enabled = true;
    volatile bool triggered; // триггер 
    static IButton* _instance;  // Статический массив для хранения экземпляров (пины) 

    // Статический обработчик прерываний
    static void IRAM_ATTR interruptRouter();

public:

    IButton(int type, int sensorPin, int typePin, int Sig, MCP* mcp = nullptr, bool enabled = true);

    bool isTrigger();
    bool initISRService();
    
};
