#include "Log.h"
#include "Core.h"
// Инициализация статических членов
const char* Log::TAG = "";
const char* Log::type = "log";
int Log::level = 0; // 0 - без дебага, 1 - с дебагом 
MQTTc* Log::mqttClient = nullptr;