#pragma once
#include <Arduino.h>
#include <esp_log.h>
#include "MQTTC.h"

class Log {
public:
    // Тег для логов
    static const char* TAG;
    static const char* type;
    static int level;
    
    // Сохраненный экземпляр MQTTc
    static MQTTc* mqttClient;

    // Инициализация системы логирования
    static void init(esp_log_level_t level = ESP_LOG_DEBUG) {
        Serial.begin(115200);
        // Сохраняем экземпляр MQTTc
        mqttClient = &MQTTc::getInstance();
        
        // Устанавливаем пользовательский обработчик логов
        esp_log_set_vprintf(mqtt_log);
        
        // Устанавливаем уровень логирования по умолчанию
        esp_log_level_set("*", level);
    }

    static void L(const char* format, ...) {
        //if (esp_log_default_level < ESP_LOG_VERBOSE) return;
        type = "log";
        va_list args;va_start(args, format);esp_log_writev(ESP_LOG_DEBUG, TAG, format, args);va_end(args);
    }

    static void E(const char* format, ...) {
        type = "error";
        va_list args;va_start(args, format);esp_log_writev(ESP_LOG_DEBUG, TAG, format, args);va_end(args);
    }

    static void D(const char* format, ...) {
        if (level == 0) return;
        type = "debug";
        va_list args;va_start(args, format);esp_log_writev(ESP_LOG_DEBUG, TAG, format, args);va_end(args);
    }

private:
    // Обработчик логов для отправки в MQTT
    static int mqtt_log(const char* format, va_list args) {
        char buffer[512];
        
        va_list copy;
        va_copy(copy, args);
        int len = vsnprintf(buffer, sizeof(buffer), format, copy);
        va_end(copy);

        if (len > 0) {
            mqttClient->log(type, String(buffer));
            // Всегда сначала выводим в Serial, чтобы загрузочные логи были видны.
            Serial.println(buffer);
        }
        if (Log::type != "log") Log::type = "log";
        return len;
    }
};
