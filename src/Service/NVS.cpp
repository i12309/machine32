#include "NVS.h"

/**
 * Реализация Singleton.
 */
NVS& NVS::getInstance() {
    static NVS instance;
    return instance;
}

/**
 * Конструктор.
 */
NVS::NVS() {}

/**
 * Инициализация NVS.
 */
void NVS::init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Log::L("NVS initialization error, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Для wifi-config пространство имен будет использоваться отдельно
    if (!preferences.begin("default", false)) {
        Log::L("Failed to initialize NVS!");
    } else {
        preferences.end();
    }
}

/**
 * Сохранить строку.
 */
bool NVS::setString(const char* key, const String& value, const char* namespace_) {
    if (!preferences.begin(namespace_, false)) {
        Log::L("NVS begin failed for namespace: %s", namespace_);
        return false;
    }
    bool result = preferences.putString(key, value);
    preferences.end();
    return result;
}

/**
 * Получить строку.
 */
String NVS::getString(const char* key, const String& defaultValue, const char* namespace_) {
    if (!preferences.begin(namespace_, true)) {
        Log::L("NVS begin failed for namespace: %s", namespace_);
        return defaultValue;
    }
    String result = preferences.getString(key, defaultValue);
    preferences.end();
    return result;
}

/**
 * Сохранить int.
 */
bool NVS::setInt(const char* key, int value, const char* namespace_) {
    if (!preferences.begin(namespace_, false)) {
        Log::L("NVS begin failed for namespace: %s", namespace_);
        return false;
    }
    bool result = preferences.putInt(key, value);
    preferences.end();
    return result;
}

/**
 * Получить int.
 */
int NVS::getInt(const char* key, int defaultValue, const char* namespace_) {
    if (!preferences.begin(namespace_, true)) {
        Log::L("NVS begin failed for namespace: %s", namespace_);
        return defaultValue;
    }
    int result = preferences.getInt(key, defaultValue);
    preferences.end();
    return result;
}

/**
 * Сохранить float.
 */
bool NVS::setFloat(const char* key, float value, const char* namespace_) {
    if (!preferences.begin(namespace_, false)) {
        Log::L("NVS begin failed for namespace: %s", namespace_);
        return false;
    }
bool result = preferences.putFloat(key, value);
    preferences.end();
    return result;
}

/**
 * Получить float.
 */
float NVS::getFloat(const char* key, float defaultValue, const char* namespace_) {
    if (!preferences.begin(namespace_, true)) {
        Log::L("NVS begin failed for namespace: %s", namespace_);
        return defaultValue;
    }
    float result = preferences.getFloat(key, defaultValue);
    preferences.end();
    return result;
}

/**
 * Сохранить байты.
 */
bool NVS::setBytes(const char* key, const void* value, size_t length, const char* namespace_) {
    if (!preferences.begin(namespace_, false)) {
        Log::L("NVS begin failed for namespace: %s", namespace_);
        return false;
    }
    bool result = preferences.putBytes(key, value, length);
    preferences.end();
    return result;
}

/**
 * Получить байты.
 */
bool NVS::getBytes(const char* key, void* buffer, size_t length, const char* namespace_) {
    if (!preferences.begin(namespace_, true)) {
        Log::L("NVS begin failed for namespace: %s", namespace_);
        return false;
    }
    size_t len = preferences.getBytesLength(key);
    if (len == length) {
        preferences.getBytes(key, buffer, length);
        preferences.end();
        return true;
    }
    preferences.end();
    return false;
}

/**
 * Проверить существование ключа.
 */
bool NVS::exists(const char* key, const char* namespace_) {
    if (!preferences.begin(namespace_, true)) {
        Log::L("NVS begin failed for namespace: %s", namespace_);
        return false;
    }
    bool result = preferences.isKey(key);
    preferences.end();
    return result;
}

/**
 * Удалить ключ.
 */
bool NVS::remove(const char* key, const char* namespace_) {
    if (!preferences.begin(namespace_, false)) {
        Log::L("NVS begin failed for namespace: %s", namespace_);
        return false;
    }
    bool result = preferences.remove(key);
    preferences.end();
    return result;
}

/**
 * Очистить пространство имен.
 */
bool NVS::clearNamespace(const char* namespace_) {
    if (!preferences.begin(namespace_, false)) {
        Log::L("NVS begin failed for namespace: %s", namespace_);
        return false;
    }
    bool result = preferences.clear();
    preferences.end();
    return result;
}
