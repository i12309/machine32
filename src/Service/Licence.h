#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WiFi.h>

#include "Log.h"

class Licence {
private:
    // Структура для хранения лицензии
    struct LicenceData {
        unsigned long valid_from; // Начало действия
        unsigned long valid_to;   // Окончание действия
        int max_usage;          // Максимальное использование
        String licence_type;    // Тип лицензии
        String features;        // Дополнительные функции (JSON)
        bool is_active;         // Активна ли лицензия
    };
    
    // Структура для хранения метрик
    struct Metrics {
        int steps;      // Количество шагов
        int cycles;     // Количество листов/циклов
        int cuts;      // Количество резов
    };
    
    LicenceData licence;
    Metrics metrics;
    bool isLoaded;
    Preferences preferences;
    
    // Приватный конструктор (синглтон)
    Licence() : isLoaded(false) {
        loadFromPreferences();
    }
    
    // Запрещаем копирование
    Licence(const Licence&) = delete;
    Licence& operator=(const Licence&) = delete;
    
    // Вспомогательная функция для парсинга времени из строки
    unsigned long parseTime(const String& timeStr) {
        if (timeStr.length() == 0) return 0;
        
        struct tm tm;
        memset(&tm, 0, sizeof(tm));
        
        // ESP32 не имеет strptime, поэтому реализуем простой парсер
        int year, month, day, hour, minute, second;
        if (sscanf(timeStr.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6 ||
            sscanf(timeStr.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
            tm.tm_year = year - 1900;
            tm.tm_mon = month - 1;
            tm.tm_mday = day;
            tm.tm_hour = hour;
            tm.tm_min = minute;
            tm.tm_sec = second;
            return static_cast<unsigned long>(mktime(&tm));
        }
        
        return 0;
    }
    
    // Вспомогательная функция для форматирования времени
    String formatTime(unsigned long time) {
        if (time == 0) return "";
        
        time_t t = static_cast<time_t>(time);
        struct tm timeinfo;
        char buffer[20];
        localtime_r(&t, &timeinfo);
        strftime(buffer, sizeof(buffer), "%d.%m.%Y", &timeinfo);
        return String(buffer);
    }

public:
    // Получаем единственный экземпляр (синглтон)
    static Licence& getInstance() {
        static Licence instance;
        return instance;
    }
    
    // Загружает лицензию из JSON
    bool loadFromJSON(const char* json) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, json);
        
        if (error) {
            Log::E("deserializeJson() failed: %s", error.c_str());
            return false;
        }
        
        licence.valid_from = parseTime(doc["valid_from"] | "");
        licence.valid_to = parseTime(doc["valid_to"] | "");
        licence.max_usage = doc["max_usage"] | 0;
        licence.licence_type = doc["licence_type"] | "";
        licence.features = doc["features"].as<String>();
        licence.is_active = doc["is_active"] | false;

        isLoaded = true;
        Log::L("Лицензия получена. licence_type: %s", licence.licence_type);
        return true;
    }
    
    // Сохраняет лицензию в Preferences
    bool saveToPreferences() {
        if (!isLoaded) {
            Serial.println("Licence not loaded, cannot save to Preferences");
            return false;
        }
        
        preferences.begin("licence", false);
        
        // Сохраняем основные данные лицензии
        preferences.putULong("valid_from", licence.valid_from);
        preferences.putULong("valid_to", licence.valid_to);
        preferences.putInt("max_usage", licence.max_usage);
        preferences.putString("type", licence.licence_type);
        preferences.putString("features", licence.features);
        preferences.putBool("active", licence.is_active);
        
        // Сохраняем метрики (имена ключей <= 15 символов для Preferences)
        preferences.putInt("metric_steps", metrics.steps);
        preferences.putInt("metric_cycles", metrics.cycles);
        preferences.putInt("metric_cuts", metrics.cuts);
        
        preferences.end();
        return true;
    }
    
    // Загружает лицензию из Preferences
    bool loadFromPreferences() {
        preferences.begin("licence", true);
        
        // Загружаем основные данные лицензии
        licence.valid_from = preferences.getULong("valid_from", 0);
        licence.valid_to = preferences.getULong("valid_to", 0);
        licence.max_usage = preferences.getInt("max_usage", 0);
        licence.licence_type = preferences.getString("type", "");
        licence.features = preferences.getString("features", "");
        licence.is_active = preferences.getBool("active", false);
        
        // Загружаем метрики
        metrics.steps = preferences.getInt("metric_steps", 0);
        metrics.cycles = preferences.getInt("metric_cycles", 0);
        metrics.cuts = preferences.getInt("metric_cuts", 0);
        
        preferences.end();
        
        isLoaded = (licence.licence_type.length() > 0);
        return isLoaded;
    }
    
    // Проверяет валидность лицензии
    bool isValid() {
        if (!isLoaded || !licence.is_active) {
            // Если флаг отключения проверки установлен, лицензия не блокирует запуск.
            return Core::settings.LICENCE_OFF == 1;
        }
        
        // Проверяем срок действия
        //time_t now = time(nullptr);
        //if (licence.valid_to != 0 && now > static_cast<time_t>(licence.valid_to)) {return false;}
        
        // Проверяем использование
        //if (licence.max_usage != 0 && licence.current_usage >= licence.max_usage) {return false;}
        
        return true;
    }
    
    // Получает значение метрики по имени
    int getMetric(const String& name) {
        if (name == "metric_steps") return metrics.steps;
        if (name == "metric_cycles") return metrics.cycles;
        if (name == "metric_cuts") return metrics.cuts;
        return -1; // Неизвестная метрика
    }
    
    // Увеличивает значение метрики
    void incrementMetric(const String& name, int value = 1) {
        if (name == "metric_step") metrics.steps += value;
        else if (name == "metric_cycles") metrics.cycles += value;
        else if (name == "metric_cuts") metrics.cuts += value;
        
        // Сохраняем изменения в Preferences
        saveMetricsToPreferences();
    }
    
    // Сохраняет только метрики в Preferences
    bool saveMetricsToPreferences() {
        preferences.begin("licence", false);
        preferences.putInt("metric_step", metrics.steps);
        preferences.putInt("metric_cycles", metrics.cycles);
        preferences.putInt("metric_cuts", metrics.cuts);
        preferences.end();
        return true;
    }
    
    // Загружает только метрики из Preferences
    bool loadMetricsFromPreferences() {
        preferences.begin("licence", true);
        metrics.steps = preferences.getInt("metric_step", 0);
        metrics.cycles = preferences.getInt("metric_cycles", 0);
        metrics.cuts = preferences.getInt("metric_cuts", 0);
        preferences.end();
        return true;
    }
    
    // Конвертирует метрики в JSON
    String metricsToJson() {
        JsonDocument doc;
        doc["metric_step"] = metrics.steps;
        doc["metric_cycles"] = metrics.cycles;
        doc["metric_cuts"] = metrics.cuts;
        
        String output;
        serializeJson(doc, output);
        return output;
    }
    
    // Получает текущую лицензию в JSON
    String toJson() {
        JsonDocument doc;
        doc["valid_from"] = formatTime(licence.valid_from);
        doc["valid_to"] = formatTime(licence.valid_to);
        doc["max_usage"] = licence.max_usage;
        doc["licence_type"] = licence.licence_type;
        doc["features"] = licence.features;
        doc["is_active"] = licence.is_active;
        
        String output;
        serializeJson(doc, output);
        return output;
    }
    
    // Проверяет, загружена ли лицензия
    bool isLicenceLoaded() {
        return isLoaded;
    }
    
    // Получает тип лицензии
    String getLicenceType() {
        return licence.licence_type;
    }
    
    // Возвращает общую информацию о лицензии в виде строки
    String getSummary() {
        String info;
        if (licence.licence_type.length() > 0) {
            info += "Тип: " + licence.licence_type;
        }
        if (licence.valid_from > 0) {
            info += "\r\n";
            info += "От: " + formatTime(licence.valid_from);
        }
        if (licence.valid_to > 0) {
            info += "\r\n";
            info += "До: " + formatTime(licence.valid_to);
        }
        if (licence.max_usage > 0) {
            info += "\r\n";
            info += "Макс. использование: " + String(licence.max_usage);
        }
        info += "\r\n";
        info += "Действующая: " + String(licence.is_active ? "да" : "нет");
        return info;
    }

    /*String getStatusInfo() {
        String status = "Licence Status:\n";
        status += "Active: " + String(licence.is_active ? "YES" : "NO") + "\r\n";
        
        if (licence.valid_to > 0) {
            time_t now = time(nullptr);
            if (now > static_cast<time_t>(licence.valid_to)) {
                status += "Status: EXPIRED\n";
            } else {
                status += "Status: VALID\n";
            }
        }
        
        return status;
    }*/
};

