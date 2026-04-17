#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <string>

#include "config.h"
#include "Machine/MachineSpec.h"
#include "Service/Log.h"

class T{
    public:
    typedef std::function<void(void)> THandlerFunction;
    typedef std::function<void(void* ptr)> THandlerPageFunction;

    // String conversion helpers (canonical type: std::string)
    static std::string toStd(const String& s) {
        return std::string(s.c_str());
    }
    static std::string toStd(const char* s) {
        return s ? std::string(s) : std::string();
    }
    static String toString(const std::string& s) {
        return String(s.c_str());
    }

    static bool isStringValidFloat(const char* str) {
        if (str == nullptr || *str == '\0') return false; // Проверка на пустую строку

        char* endPtr;
        float value = strtof(str, &endPtr); // Преобразование строки в float

        // Если endPtr указывает на начало строки, значит преобразование не удалось
        if (endPtr == str) {
            return false; // Строка не содержит допустимых чисел
        }

        // Проверка: если остались непреобразованные символы после числа
        while (*endPtr != '\0') {
            if (!isspace(*endPtr)) { // Пропускаем пробельные символы
                return false; // Недопустимые символы после числа
            }
            endPtr++;
        }

        return true; // Преобразование успешное
    }
    static bool isStringValidInteger(const String& str) {
        if (str.isEmpty()) return false; // Проверка на пустую строку

        bool hasDigit = false; // Флаг наличия хотя бы одной цифры
        int index = 0;

        // Проверка на знак (+ или -)
        if (str[index] == '+' || str[index] == '-') {
            index++;
        }

        // Проверка оставшейся части строки
        for (; index < str.length(); index++) {
            char c = str[index];

            // Разрешаем только цифры
            if (isdigit(c)) {
                hasDigit = true;
                continue;
            }

            // Все остальные символы недопустимы
            return false;
        }

        return hasDigit; // Должна быть хотя бы одна цифра
    }
};

class FileSystem{ public:
    static bool init(bool formatOnFail = true) {
        if (!LittleFS.begin(true, "/spiffs")) {
            if (formatOnFail) {
                if (LittleFS.format()) {
                    Log::D("LittleFS отформатирована и инициализирована успешно");
                    return LittleFS.begin();
                } else {
                    Log::D("Ошибка форматирования LittleFS");
                    return false;
                }
            }
            Log::D("Ошибка инициализации LittleFS");
            return false;
        }
        Log::D("LittleFS инициализирован успешно");
        return true;
    }

    // Чтение JSON-файла и десериализация в структуру
    static bool readConfig(String& formattedJson) {
        return readJsonFile(CONFIG_PATH, formattedJson);
    }

    static bool readData(String& formattedJson) {
        return readJsonFile(DATA_PATH, formattedJson);
    }

    // Сохранение JSON-файла, сериализованного из структуры
    static bool saveConfig(String& data) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data);
        if (error) {
            Log::D("Ошибка парсинга config JSON: %s", error.c_str());
            return false;
        }

        if (!writeJsonDocument(CONFIG_PATH, doc)) {
            return false;
        }

        Log::D("JSON сохранен в файл '%s'", CONFIG_PATH.c_str());
        return true;
    }

    static bool saveData(String& data) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data);
        if (error) {
            Log::D("Ошибка парсинга data JSON: %s", error.c_str());
            return false;
        }

        if (!writeJsonDocument(DATA_PATH, doc)) {
            return false;
        }

        Log::D("JSON сохранен в файл '%s'", DATA_PATH.c_str());
        return true;
    }

    // Удаление файла
    static bool deleteFile(const char* filename) {
        if (!LittleFS.exists(filename)) {
            Serial.printf("Файл '%s' не существует\n", filename);
            return false;
        }

        if (LittleFS.remove(filename)) {
            Serial.printf("Файл '%s' удален\n", filename);
            return true;
        } else {
            Serial.printf("Ошибка удаления файла '%s'\n", filename);
            return false;
        }
    }

private:
    static bool readJsonDocument(const String& path, JsonDocument& doc) {
        if (!LittleFS.exists(path)) {
            return false;
        }

        File file = LittleFS.open(path, "r");
        if (!file) {
            return false;
        }

        doc.clear();
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        if (error) {
            return false;
        }
        return true;
    }

    static bool readJsonFile(const String& path, String& formattedJson) {
        JsonDocument doc;
        if (!readJsonDocument(path, doc)) {
            Log::D("Файл '%s' не найден или поврежден", path.c_str());
            return false;
        }

        serializeJsonPretty(doc, formattedJson);
        Log::D("JSON из файла '%s' успешно загружен", path.c_str());
        return true;
    }

    static bool writeJsonDocument(const String& path, const JsonDocument& doc) {
        File file = LittleFS.open(path, "w");
        if (!file) {
            Log::D("Ошибка открытия файла '%s' для записи", path.c_str());
            return false;
        }

        serializeJsonPretty(doc, file);
        file.close();
        return true;
    }
};
class Core {
private:
    struct DataJson {
        JsonDocument doc;

        bool load() {
            Log::D(__func__);

            doc.clear();
            if (!LittleFS.exists(DATA_PATH)) {
                return true;
            }

            File file = LittleFS.open(DATA_PATH, "r");
            if (!file) {
                Log::D("Ошибка открытия файла '%s'", DATA_PATH.c_str());
                return false;
            }

            if (file.size() == 0) {
                file.close();
                return true;
            }

            DeserializationError error = deserializeJson(doc, file);
            file.close();

            if (error) {
                Log::D("Ошибка парсинга data.json: %s", error.c_str());
                return false;
            }
            if (!doc["profiles"].is<JsonArray>()) {
                doc["profiles"].to<JsonArray>();
            }
            if (!doc["tasks"].is<JsonArray>()) {
                doc["tasks"].to<JsonArray>();
            }

            return true;
        }

        bool save() {
            File file = LittleFS.open(DATA_PATH, "w");
            if (!file) {
                Log::D("Ошибка записи data.json");
                return false;
            }
            serializeJsonPretty(doc, file);
            file.close();
            return true;
        }
    };
public: static DataJson data;

private:
    struct ConfigJson {
        JsonDocument doc;
        String machine;
        int configVersion;
        String name;
        String group;

        // записать дефолтный config.json
        bool writeDefault() {
            ConfigDefaults::Options options;
            if (!ConfigDefaults::build(doc, options)) {
                Log::D("Failed to build default config from MachineSpec");
                return false;
            }
            machine = Catalog::machineName(options.machine);
            configVersion = options.configVersion;
            name = options.name;
            group = options.group;

            if (!settings.load(doc)) {
                Log::D("Ошибка загрузки settings из дефолтного config");
                return false;
            }

            File file = LittleFS.open(CONFIG_PATH, "w");
            if (!file) {
                Log::D("Ошибка создания файла");
                return false;
            }
            serializeJsonPretty(doc, file);
            file.close();
            return true;
        }

        // открыть, если его нет, то создать по умолчанию (если разрешено)
        bool open(File& file, bool createDefaultIfMissing){
            if (!LittleFS.exists(CONFIG_PATH)) {
                if (!createDefaultIfMissing) {
                    Log::D("Файл '%s' не найден", CONFIG_PATH.c_str());
                    return false;
                }
                Log::D("Файл не найден, создаем из дефолтного");
                if (!writeDefault()) return false;
            }

            file = LittleFS.open(CONFIG_PATH, "r");
            if (!file) {
                Log::D("Ошибка открытия файла '%s'", CONFIG_PATH.c_str());
                return false;
            }

            if (file.size() == 0) {
                file.close();
                if (!createDefaultIfMissing) {
                    Log::D("Файл '%s' пустой", CONFIG_PATH.c_str());
                    return false;
                }
                Log::D("Файл пустой, создаем из дефолтного");
                if (!writeDefault()) return false;
                file = LittleFS.open(CONFIG_PATH, "r");
                if (!file || file.size() == 0) {
                    Log::D("Ошибка открытия после создания");
                    return false;
                }
            }
            return true;
        }

        // Единый метод загрузки конфигурации
        bool load(bool createDefaultIfMissing = true) {
            Log::D(__func__);

            File file;
            if (!open(file, createDefaultIfMissing)) return false;

            DeserializationError error = deserializeJson(doc, file);
            file.close();

            if (error) {
                Log::D("Ошибка парсинга config.json: %s", error.c_str());
                return false;
            }

            // Загрузка переменной machine
            machine = doc["machine"] | "UNKNOWN";
            configVersion = doc["config_version"] | 0;
            name = doc["name"] | "ESP32";
            group = doc["group"] | "";

            // Загрузка основных параметров
            if (!settings.load(doc)) {
                Log::D("Ошибка загрузки секции settings");
                return false;
            }
            Catalog::MachineType machineType = Catalog::getMachine(machine);
            MachineSpec spec = MachineSpec::get(machineType);
            JsonObjectConst deviceObj = doc["device"];
            MachineSpec::Report report = spec.validateDeviceConfig(deviceObj);

            for (const String& warning : report.warnings) {
                Log::D("%s", warning.c_str());
            }
            if (report.hasErrors()) {
                for (const String& err : report.errors) {
                    Log::E("%s", err.c_str());
                }
                Log::E("[Config] MachineSpec validation failed for machine='%s'", machine.c_str());
                return false;
            }

            return true;
        }

        // сохранение
        bool save() {
            // Сохраняем переменные из корня config
            doc["machine"] = machine;
            doc["config_version"] = configVersion;
            doc["name"] = name;
            doc["group"] = group;

            JsonObject settingsObj = doc["settings"];
            settings.serialize(settingsObj);

            File file = LittleFS.open(CONFIG_PATH, "w");
            if (!file) {
                Log::D("Ошибка записи файла");
                return false;
            }
            serializeJsonPretty(doc, file);
            file.close();
            return true;
        }

        // Отладка: вывод конфига в терминал
        void print_config() {
            if (Log::level == 1){
                Log::L("========== CONFIG DEBUG ==========");
                serializeJsonPretty(doc, Serial);
                Serial.println();
                Log::L("==================================");
            }
        }
    };
public: static ConfigJson config;

private: struct Settings {
        int AUTO_UPDATE; // автообновление
        String SERVER;
        String VERSION;
        String FIRMWARE;
        String HASH;
        int UPDATE;

        String TFT_VERSION;
        String TFT_FIRMWARE;
        int TFT_UPDATE;

        int CONNECT_WIFI;// 1
        int ACCESS_POINT;// 0
        int HTTP_SERVER;// 1
        int WEB;// 0
        int log;// 0

        int LOG_BUFFER;// 0

        int CHECK_SYSTEM;// 0
        int ALLOW_MISSING_HARDWARE;// 0
        int LICENCE_OFF;// 1

        int metrics;// 0 - статистика


        int MCP_TRIGGER_I2C_WAIT_MS;// Таймаут ожидания I2C для arm/disarm/read snapshot в McpTrigger.

        String buildLevel(const String& fileName, int level) const {
            switch (level) {
                case 1: return "http://" + SERVER + "/" + Core::config.machine + "/" + fileName;
                case 2: return "http://" + SERVER + "/" + Core::config.machine + "/" + Core::config.group + "/" + fileName;
                case 3: return "http://" + SERVER + "/" + Core::config.machine + "/" + Core::config.group + "/" + Core::config.name + "/" + fileName;
                default: return "";
            }
        }

        // Функции для сборки URL
        String getVersionURL(int level = 3) const { return buildLevel(VERSION, level); }
        String getFirmwareURL(int level = 3) const { return buildLevel(FIRMWARE, level); }
        String getHashURL(int level = 3) const { return buildLevel(HASH, level); }
        String getTFTVersionURL(int level = 3) const { return buildLevel(TFT_VERSION, level); }
        String getTFTFirmwareURL(int level = 3) const { return buildLevel(TFT_FIRMWARE, level); }

        // Загрузка секции settings
        bool load(const JsonDocument& doc) {

            JsonObjectConst obj = doc["settings"];
            if (obj.isNull()) {
                Log::D("Объект settings не найден");
                return false;
            }

            settings.log = obj["log"] | 0;
            Log::level = settings.log;

            settings.AUTO_UPDATE = obj["AUTO_UPDATE"] | 0;

            settings.SERVER = obj["SERVER"] | "";
            settings.VERSION = obj["VERSION"] | "";
            settings.FIRMWARE = obj["FIRMWARE"] | "";
            settings.HASH = obj["HASH"] | "";
            settings.UPDATE = obj["UPDATE"] | 0;

            settings.TFT_VERSION = obj["TFT_VERSION"] | "";
            settings.TFT_FIRMWARE = obj["TFT_FIRMWARE"] | "";
            settings.TFT_UPDATE = obj["TFT_UPDATE"] | 0;

            settings.CONNECT_WIFI = obj["CONNECT_WIFI"] | 0;
            settings.ACCESS_POINT = obj["ACCESS_POINT"] | 0;
            settings.HTTP_SERVER = obj["HTTP_SERVER"] | 0;
            // WEB строго бинарный флаг: 0 (выключен) или 1 (включен).
            // По умолчанию (если параметр отсутствует) - 0.
            settings.WEB = ((obj["WEB"] | 0) == 1) ? 1 : 0;

            settings.LOG_BUFFER = obj["LOG_BUFFER"] | 0;
            settings.CHECK_SYSTEM = obj["CHECK_SYSTEM"] | 0;
            settings.ALLOW_MISSING_HARDWARE = obj["ALLOW_MISSING_HARDWARE"] | 0;
            settings.LICENCE_OFF = obj["licence_off"] | 1;
            settings.metrics = obj["metrics"] | 0;
            // Если параметр отсутствует, используем тот же дефолт, что и в config defaults.
            settings.MCP_TRIGGER_I2C_WAIT_MS = obj["MCP_TRIGGER_I2C_WAIT_MS"] | 150;

            return true;
        }

        void serialize(JsonObject& obj) {
            obj["AUTO_UPDATE"] = settings.AUTO_UPDATE;
            obj["SERVER"] = settings.SERVER;
            obj["VERSION"] = settings.VERSION;
            obj["FIRMWARE"] = settings.FIRMWARE;
            obj["HASH"] = settings.HASH;
            obj["UPDATE"] = settings.UPDATE;
            obj["TFT_VERSION"] = settings.TFT_VERSION;
            obj["TFT_FIRMWARE"] = settings.TFT_FIRMWARE;
            obj["TFT_UPDATE"] = settings.TFT_UPDATE;
            obj["CONNECT_WIFI"] = settings.CONNECT_WIFI;
            obj["ACCESS_POINT"] = settings.ACCESS_POINT;
            obj["HTTP_SERVER"] = settings.HTTP_SERVER;
            obj["WEB"] = settings.WEB;
            obj["log"] = settings.log;
            obj["LOG_BUFFER"] = settings.LOG_BUFFER;
            obj["CHECK_SYSTEM"] = settings.CHECK_SYSTEM;
            obj["ALLOW_MISSING_HARDWARE"] = settings.ALLOW_MISSING_HARDWARE;
            obj["licence_off"] = settings.LICENCE_OFF;
            obj["metrics"] = settings.metrics;
            obj["MCP_TRIGGER_I2C_WAIT_MS"] = settings.MCP_TRIGGER_I2C_WAIT_MS;
        }
    };
public: static Settings settings;
};
