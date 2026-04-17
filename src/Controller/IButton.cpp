#include "IButton.h"

// Инициализация статического массива
IButton* IButton::_instance = nullptr;

// Статический обработчик прерываний
void IRAM_ATTR IButton::interruptRouter() {
    static volatile uint32_t lastInterruptTime = 0;
    uint32_t currentTime = millis();
    
    if (currentTime - lastInterruptTime < 500) {
        return;
    }
    lastInterruptTime = currentTime;

    // Проверяем состояние пина, вызвавшего прерывание
    if (digitalRead(_instance->_sensorPin) == _instance->_Sig) {
        //Log::D("Сработало! pin = %d",_instance->_sensorPin);
        _instance->triggered = true;
    }
}

IButton::IButton(int type, int sensorPin, int typePin, int Sig, MCP* mcp, bool enabled) : 
_sensorPin(sensorPin), 
_typePin(typePin), 
_Sig(Sig),
_enabled(enabled),
triggered(false)
{   /*
    _use_mcp = false;
    if (mcp != nullptr){
        _use_mcp = true;
        _mcp = mcp;
        // Кнопка подключена через MCP23017
        Log::D("Кнопка будет работать через MCP23017 '%s'", _mcp->getName().c_str());
        // Прерывания будут обрабатываться через IExtender::processInterrupts()

    } else {
        // Кнопка подключена напрямую к ESP32
        // 1. Проверяем валидность пина
        if (_sensorPin < 0 || _sensorPin >= GPIO_NUM_MAX) {
            Log::D("ОШИБКА: Неверный номер пина %d", _sensorPin);
            return;
        }
        
        // 2. Проверяем поддержку прерываний для пина
        if (!digitalPinCanOutput(_sensorPin)) {
            Log::D("ПРЕДУПРЕЖДЕНИЕ: Pin %d может не поддерживать все функции", _sensorPin);
        }
        
        // 3. Безопасная настройка pinMode
        pinMode(_sensorPin, _typePin);
        delay(10); // Небольшая задержка для стабилизации
        
        // 4. Регистрируем экземпляр
        _instance = this;
        
        // 5. Инициализируем состояние
        triggered = (digitalRead(_sensorPin) == !_Sig);
        
        // 6. Проверяем доступность функции digitalPinToInterrupt
        /*int interruptNum = digitalPinToInterrupt(_sensorPin);
        Log::D("Номер прерывания для pin %d = %d", _sensorPin, interruptNum);
        
        if (interruptNum == NOT_AN_INTERRUPT) {
            Log::D("ОШИБКА: Pin %d не поддерживает прерывания!", _sensorPin);
            return;
        }*/
   /*     
        // 7. Отключаем возможное предыдущее прерывание
        //detachInterrupt(digitalPinToInterrupt(_sensorPin));
        //delay(10);
        
        // 8. Подключаем прерывание
        attachInterrupt(digitalPinToInterrupt(_sensorPin), interruptRouter, CHANGE);
        
        Log::D("Прерывание успешно подключено для pin %d", _sensorPin);
    }
    
    Log::D("Кнопка создана успешно!");
    triggered = false;
    /**/
};

bool IButton::isTrigger() {
    if (!_enabled) return false;
    bool check = (triggered == true);
    triggered = false;
    return check;
}
