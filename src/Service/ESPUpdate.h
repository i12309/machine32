#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <MD5Builder.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_log.h>
#include <functional>

#include "config.h"
#include "Core.h"
#include "Log.h"

static const char *OTA_TAG = "ESP_OTA";

/**
 * Класс для управления OTA-обновлениями ESP32.
 * Предоставляет методы для проверки, скачивания и применения обновлений прошивки.
 */
class ESPUpdate {
public:
    using ProgressCallback = std::function<void(int)>;
    /**
     * Получить единственный экземпляр класса (Singleton).
     * @return Ссылка на экземпляр ESPUpdate.
     */
    static ESPUpdate& getInstance();

    /**
     * Проверить наличие новой версии прошивки ESP32.
     * @return true, если есть новая версия; false иначе.
     */
    int checkForUpdate();

    /**
     * Проверить наличие новой версии прошивки Nextion.
     * @param vTFT Текущая версия TFT.
     * @return true, если есть новая версия; false иначе.
     */
    bool checkForUpdate_Nextion(int vTFT);

    /**
     * Получить новую версию приложения (строкой).
     * @return Новая версия в виде строки.
     */
    String getNewAppVersion();

    /**
     * Выполнить обновление прошивки ESP32.
     * @return true, если обновление успешно; false иначе.
     */
    bool FirmwareUpdate(int level = 2, ProgressCallback progress = nullptr);


    /**
     * Переключить OTA-раздел для следующей загрузки.
     */
    void switchOTAPartition();
    void markNewFirmwarePendingValidation();
    void markCurrentFirmwareValid();
    bool rollbackToPreviousPartition(const char* reason = nullptr);

private:
    String appNewVersion;  ///< Новая версия приложения для последующего использования.

    /**
     * Приватный конструктор (Singleton).
     */
    ESPUpdate() = default;

    /**
     * Запрет копирования (Singleton).
     */
    ESPUpdate(const ESPUpdate&) = delete;
    ESPUpdate& operator=(const ESPUpdate&) = delete;

    /**
     * Получить MD5-хэш прошивки с сервера.
     * @param md5 Выходной параметр: MD5-хэш.
     * @return true, если хэш получен; false иначе.
     */
    bool getMD5Hash(String& md5, int level = 2);

    /**
     * Откат на предыдущую версию прошивки.
     */
    void rollbackUpdate();


};
