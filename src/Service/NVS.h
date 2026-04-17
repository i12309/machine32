#pragma once

#include <Preferences.h>
#include <nvs_flash.h>
#include "Log.h"

/**
 * Класс для универсального управления NVS (Non-Volatile Storage).
 * Обеспечивает сохранение, загрузку и удаление переменных различных типов.
 */
class NVS {
public:
    /**
     * Получить единственный экземпляр klasy (Singleton).
     * @return Ссылка на экземпляр NVS.
     */
    static NVS& getInstance();

    /**
     * Инициализировать NVS. Вызывается автоматически в конструкторе, но можно переинициализировать.
     */
    void init();

    /**
     * Сохранить строку в NVS.
     * @param key Ключ для сохранения.
     * @param value Значение строки.
     * @param namespace_ Пространство имен (по умолчанию "default").
     * @return true, если сохранение успешно; false иначе.
     */
    bool setString(const char* key, const String& value, const char* namespace_ = "default");

    /**
     * Получить строку из NVS.
     * @param key Ключ для получения.
     * @param defaultValue Значение по умолчанию, если ключ не найден.
     * @param namespace_ Пространство имен (по умолчанию "default").
     * @return Значение строки или defaultValue.
     */
    String getString(const char* key, const String& defaultValue = "", const char* namespace_ = "default");

    /**
     * Сохранить целое число в NVS.
     * @param key Ключ для сохранения.
     * @param value Значение числа.
     * @param namespace_ Пространство имен (по умолчанию "default").
     * @return true, если сохранение успешно; false иначе.
     */
    bool setInt(const char* key, int value, const char* namespace_ = "default");

    /**
     * Получить целое число из NVS.
     * @param key Ключ для получения.
     * @param defaultValue Значение по умолчанию, если ключ не найден.
     * @param namespace_ Пространство имен (по умолчанию "default").
     * @return Значение числа или defaultValue.
     */
    int getInt(const char* key, int defaultValue = 0, const char* namespace_ = "default");

    /**
     * Сохранить float в NVS.
     * @param key Ключ для сохранения.
     * @param value Значение float.
     * @param namespace_ Пространство имен (по умолчанию "default").
     * @return true, если сохранение успешно; false иначе.
     */
    bool setFloat(const char* key, float value, const char* namespace_ = "default");

    /**
     * Получить float из NVS.
     * @param key Ключ для получения.
     * @param defaultValue Значение по умолчанию, если ключ не найден.
     * @param namespace_ Пространство имен (по умолчанию "default").
     * @return Значение float или defaultValue.
     */
    float getFloat(const char* key, float defaultValue = 0.0f, const char* namespace_ = "default");

    /**
     * Сохранить бинарные данные в NVS.
     * @param key Ключ для сохранения.
     * @param value Указатель на данные.
     * @param length Длина данных.
     * @param namespace_ Пространство имен (по умолчанию "default").
     * @return true, если сохранение успешно; false иначе.
     */
    bool setBytes(const char* key, const void* value, size_t length, const char* namespace_ = "default");

    /**
     * Получить бинарные данные из NVS.
     * @param key Ключ для получения.
     * @param buffer Буфер для данных.
     * @param length Длина буфера.
     * @param namespace_ Пространство имен (по умолчанию "default").
     * @return true, если получение успешно; false иначе.
     */
    bool getBytes(const char* key, void* buffer, size_t length, const char* namespace_ = "default");

    /**
     * Проверить, существует ли ключ.
     * @param key Ключ для проверки.
     * @param namespace_ Пространство имен (по умолчанию "default").
     * @return true, если ключ существует; false иначе.
     */
    bool exists(const char* key, const char* namespace_ = "default");

    /**
     * Удалить ключ из NVS.
     * @param key Ключ для удаления.
     * @param namespace_ Пространство имен (по умолчанию "default").
     * @return true, если удаление успешно; false иначе.
     */
    bool remove(const char* key, const char* namespace_ = "default");

    /**
     * Очистить все данные в пространстве имен.
     * @param namespace_ Пространство имен.
     * @return true, если очистка успешна; false иначе.
     */
    bool clearNamespace(const char* namespace_);

private:
    Preferences preferences;  ///< Объект для работы с Preferences (NVS).

    /**
     * Приватный конструктор (Singleton). Инициализирует NVS.
     */
    NVS();

    /**
     * Запрет копирования (Singleton).
     */
    NVS(const NVS&) = delete;
    NVS& operator=(const NVS&) = delete;

    /**
     * Деструктор. Закрывает Preferences.
     */
    ~NVS() {
        preferences.end();
    }
};