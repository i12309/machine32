#pragma once

#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <MD5Builder.h> // Для проверки контрольной суммы MD5
#include <nvs_flash.h>

#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_log.h>

#include "config.h"
#include "Core.h"
#include "Log.h"

#include "NVS.h"

static const char *TAG = "OTA_SWITCH";

class WiFiConfig {
public:

    // Получить единственный экземпляр класса.
    static WiFiConfig& getInstance() {
        static WiFiConfig instance;
        return instance;
    }

    // Возвращает количество сохраненных сетей.
    uint8_t getCount() {
        ensureLoaded();
        return store.count;
    }

    // Возвращает SSID по индексу (или пустую строку при выходе за пределы).
    const char* getSSIDByIndex(uint8_t index) {
        ensureLoaded();
        if (index >= store.count) return "";
        return store.items[index].ssid;
    }

    // Возвращает индекс сети по умолчанию.
    int getDefaultIndex() {
        ensureLoaded();
        return store.defaultIndex;
    }

    // Устанавливает индекс сети по умолчанию.
    bool setDefaultIndex(int index) {
        ensureLoaded();
        if (index < 0 || index >= store.count) {
            store.defaultIndex = -1;
            return false;
        }
        store.defaultIndex = index;
        return true;
    }

    // Возвращает индекс сети, к которой сейчас подключены, либо -1.
    int getConnectedIndex() {
        ensureLoaded();
        if (WiFi.status() != WL_CONNECTED) return -1;
        String ssid = WiFi.SSID();
        return findIndexBySsid(ssid.c_str());
    }

    // Добавляет новую сеть или обновляет пароль существующей.
    bool addOrUpdate(const char* ssid, const char* password) {
        ensureLoaded();
        if (ssid == nullptr || strlen(ssid) == 0) {
            _lastError = "SSID пустой";
            return false;
        }
        const char* safePassword = (password == nullptr) ? "" : password;

        int index = findIndexBySsid(ssid);
        if (index >= 0) {
            strncpy(store.items[index].pass, safePassword, sizeof(store.items[index].pass) - 1);
            store.items[index].pass[sizeof(store.items[index].pass) - 1] = '\0';
            if (isConnectedTo(ssid)) {
                disconnect();
            }
            return true;
        }

        if (store.count >= MAX_WIFI_NETWORKS) {
            _lastError = "Слишком много сетей";
            return false;
        }

        WiFiEntry& entry = store.items[store.count];
        memset(&entry, 0, sizeof(entry));
        strncpy(entry.ssid, ssid, sizeof(entry.ssid) - 1);
        strncpy(entry.pass, safePassword, sizeof(entry.pass) - 1);
        store.count++;
        return true;
    }

    // Удаляет сеть по SSID и сдвигает список.
    bool remove(const char* ssid) {
        ensureLoaded();
        int index = findIndexBySsid(ssid);
        if (index < 0) return false;

        if (isConnectedTo(ssid)) {
            disconnect();
        }

        for (int i = index; i < (int)store.count - 1; ++i) {
            store.items[i] = store.items[i + 1];
        }
        if (store.count > 0) store.count--;

        if (store.defaultIndex == index) {
            store.defaultIndex = -1;
        } else if (store.defaultIndex > index) {
            store.defaultIndex--;
        }

        return true;
    }

    // Полностью очищает список сетей и запись в NVS.
    bool clearAll() {
        clearStore();
        if (!openPrefs(false)) return false;
        bool result = preferences.remove("wifi");
        closePrefs();
        return result;
    }

    // Подключается к сети по умолчанию (используется при старте).
    bool begin() {
        ensureLoaded();

        if (store.count == 0 || store.defaultIndex < 0 || store.defaultIndex >= store.count) {
            if (Core::settings.ACCESS_POINT) startAccessPoint();
            return false;
        }

        const WiFiEntry& entry = store.items[store.defaultIndex];
        return connectWithCreds(entry.ssid, entry.pass, true);
    }

    // Подключается к выбранной сети из списка (без запуска AP).
    bool connectTo(const char* ssid) {
        ensureLoaded();
        int index = findIndexBySsid(ssid);
        if (index < 0) {
            _lastError = "Сеть не найдена";
            return false;
        }
        const WiFiEntry& entry = store.items[index];
        return connectWithCreds(entry.ssid, entry.pass, false);
    }

    // Разовое подключение по указанным учетным данным (без сохранения).
    bool connectOnce(const char* ssid, const char* password) {
        return connectWithCreds(ssid, password, false);
    }

    // Разрывает текущее Wi-Fi соединение.
    bool disconnect() {
        WiFi.disconnect();
        return true;
    }

    // Сохраняет список сетей в NVS.
    bool save() {
        ensureLoaded();
        if (!openPrefs(false)) {
            Log::D("Failed to initialize NVS!");
            return false;
        }

        preferences.putBytes("wifi", &store, sizeof(store));
        closePrefs();
        Log::D("Wi-Fi settings saved to NVS.");
        return true;
    }

    // Загружает список сетей из NVS.
    bool load() {
        if (!openPrefs(true)) {
            Log::D("Failed to initialize NVS!");
            clearStore();
            _loaded = true;
            return false;
        }

        size_t len = preferences.getBytesLength("wifi");
        if (len == sizeof(store)) {
            preferences.getBytes("wifi", &store, sizeof(store));
            closePrefs();
            Log::D("Wi-Fi settings loaded from NVS.");
            _loaded = true;
            return true;
        }

        closePrefs();
        Log::D("No valid Wi-Fi settings in NVS.");
        clearStore();
        _loaded = true;
        return false;
    }

    // Проверяет, подключены ли сейчас к указанному SSID.
    bool isConnectedTo(const char* ssid) {
        if (ssid == nullptr || strlen(ssid) == 0) return false;
        if (WiFi.status() != WL_CONNECTED) return false;
        return WiFi.SSID().equals(ssid);
    }

    // Возвращает факт наличия Wi-Fi соединения.
    bool isConnect(){
        return WiFi.status() == WL_CONNECTED;
    }

    // Возвращает текст последней ошибки подключения.
    const char* getLastError() {
        return _lastError.c_str();
    }

    // Возвращает строку с информацией о сети и уровне сигнала.
    String getNetInfo(){
        if (WiFi.status() != WL_CONNECTED) return "Нет сети";
        String ssid = WiFi.SSID();
        if (ssid == "") return "Нет сети";
        if (getRSSI() == 0) return "Нет связи";
        //String wifiInfo = (ssid == "") ? "Нет сети" : ssid + ": " + ((rssi == 0) ? "нет сигнала" : String(rssi));
        return ssid;
    }

    // Возвращает RSSI текущего соединения.
    int getRSSI(){ return (WiFi.RSSI() == 0) ? 0 : 100 + WiFi.RSSI();}
    // Возвращает IP текущего соединения.
    IPAddress getIP(){ return WiFi.localIP(); }

    // Возвращает последние два байта MAC в виде строки.
    String mac_xx() {
        uint8_t mac[6];
        WiFi.macAddress(mac); // Получаем MAC

        // Берём последние два байта (5-й и 6-й в массиве)
        uint8_t part1 = mac[4];
        uint8_t part2 = mac[5];

        // Форматируем в строку (без разделителей)
        char lastParts[5]; // 4 символа + нулевой терминатор
        sprintf(lastParts, "%02X%02X", part1, part2);
        return lastParts;
    }

    bool startAccessPoint() {
        if (!Core::settings.ACCESS_POINT) return false;
        // Поднимаем точку доступа 
        if (WiFi.softAP("SMIT_"+String(APP_VERSION)+"_"+mac_xx())) {
            delay(1000); // Даем время на инициализацию AP
            Log::D("Access Point started.");
            Log::D("IP Address: %s", WiFi.softAPIP().toString().c_str());
            delay(1000); // Даем время на инициализацию AP
            return true;
        } else {
            Log::D("Failed to start Access Point.");
            return false;
        }
    }

    // Пытается подключиться по указанным учетным данным.
    bool connectWithCreds(const char* ssid, const char* pass, bool allowAp) {
        _lastError = "";
        if (ssid == nullptr || strlen(ssid) == 0) {
            _lastError = "SSID пустой";
            return false;
        }

        WiFi.disconnect();
        if (pass == nullptr || strlen(pass) == 0) {
            WiFi.begin(ssid);
        } else {
            WiFi.begin(ssid, pass);
        }
        Log::D("Connecting to Wi-Fi");
        unsigned long startTime = millis();

        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - startTime > 10000) {
                wl_status_t status = WiFi.status();
                WiFi.disconnect();

                if (status == WL_CONNECT_FAILED) {
                    _lastError = "Неверный пароль или сеть недоступна";
                } else if (status == WL_NO_SSID_AVAIL) {
                    _lastError = "Сеть не найдена";
                } else if (status == WL_CONNECTION_LOST) {
                    _lastError = "Соединение потеряно";
                } else {
                    _lastError = "Ошибка подключения";
                }

                if (allowAp) startAccessPoint();
                return false;
            }
            delay(500);
            Serial.print(".");
        }

        Log::D("Connected to Wi-Fi!");
        Log::D("IP Address: %s", WiFi.localIP().toString().c_str());
        return true;
    }

private:
    Preferences preferences;
    String _lastError = "";
    bool _loaded = false;

    static constexpr uint8_t MAX_WIFI_NETWORKS = 5;

    struct WiFiEntry {
        char ssid[32];
        char pass[64];
    };

    struct WiFiStore {
        uint8_t count = 0;
        int8_t defaultIndex = -1;
        WiFiEntry items[MAX_WIFI_NETWORKS];
    };

    WiFiStore store;

    // Конструктор: сбрасывает хранилище.
    WiFiConfig() { clearStore(); }

    // Гарантирует загрузку данных из NVS один раз.
    void ensureLoaded() {
        if (!_loaded) load();
    }

    // Очищает структуру хранилища в RAM.
    void clearStore() {
        memset(&store, 0, sizeof(store));
        store.count = 0;
        store.defaultIndex = -1;
    }

    // Открывает Preferences в режиме только-чтение или чтение/запись.
    bool openPrefs(bool readOnly) {
        return preferences.begin("wifi", readOnly);
    }

    // Закрывает Preferences.
    void closePrefs() {
        preferences.end();
    }

    // Ищет индекс сети по SSID.
    int findIndexBySsid(const char* ssid) {
        if (ssid == nullptr) return -1;
        for (int i = 0; i < store.count; ++i) {
            if (strncmp(store.items[i].ssid, ssid, sizeof(store.items[i].ssid)) == 0) return i;
        }
        return -1;
    }

};
