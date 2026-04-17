#include "ESPUpdate.h"
#include "NVS.h"

namespace {
constexpr const char* kBootNamespace = "boot";
constexpr const char* kBootCountKey = "boot_count";
constexpr const char* kOtaPendingKey = "ota_pending";
}

/**
 * Реализация Singleton.
 */
ESPUpdate& ESPUpdate::getInstance() {
    static ESPUpdate instance;
    return instance;
}

void ESPUpdate::markNewFirmwarePendingValidation() {
    NVS& nvs = NVS::getInstance();
    nvs.setInt(kOtaPendingKey, 1, kBootNamespace);
    nvs.setInt(kBootCountKey, 0, kBootNamespace);
    Log::L("OTA pending validation is enabled");
}

void ESPUpdate::markCurrentFirmwareValid() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (running != nullptr) {
        esp_ota_img_states_t state;
        const esp_err_t stateErr = esp_ota_get_state_partition(running, &state);
        if (stateErr == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY) {
            const esp_err_t markErr = esp_ota_mark_app_valid_cancel_rollback();
            if (markErr == ESP_OK) {
                Log::L("Current firmware marked as VALID");
            } else {
                Log::E("Failed to mark firmware valid: %s", esp_err_to_name(markErr));
            }
        }
    }

    NVS& nvs = NVS::getInstance();
    nvs.setInt(kOtaPendingKey, 0, kBootNamespace);
    nvs.setInt(kBootCountKey, 0, kBootNamespace);
}

bool ESPUpdate::rollbackToPreviousPartition(const char* reason) {
    Log::E("Rollback requested: %s", reason ? reason : "unknown reason");

    const esp_partition_t* running = esp_ota_get_running_partition();
    if (running == nullptr) {
        Log::E("Rollback failed: running partition is null");
        return false;
    }

    esp_ota_img_states_t state;
    const esp_err_t stateErr = esp_ota_get_state_partition(running, &state);
    if (stateErr == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY) {
        const esp_err_t rollbackErr = esp_ota_mark_app_invalid_rollback_and_reboot();
        if (rollbackErr == ESP_OK) {
            return true;
        }
        Log::E("IDF rollback API failed: %s", esp_err_to_name(rollbackErr));
    }

    const esp_partition_t* target = esp_ota_get_next_update_partition(running);
    if (target == nullptr) {
        Log::E("Rollback failed: target partition not found");
        return false;
    }

    esp_app_desc_t app_desc;
    if (esp_ota_get_partition_description(target, &app_desc) != ESP_OK) {
        Log::E("Rollback failed: target partition '%s' has no valid app", target->label);
        return false;
    }

    if (esp_ota_set_boot_partition(target) != ESP_OK) {
        Log::E("Rollback failed: cannot set boot partition '%s'", target->label);
        return false;
    }

    NVS& nvs = NVS::getInstance();
    nvs.setInt(kOtaPendingKey, 0, kBootNamespace);
    nvs.setInt(kBootCountKey, 0, kBootNamespace);
    Log::L("Rollback to partition '%s' is scheduled. Restarting...", target->label);
    esp_restart();
    return true;
}

/**
 * Проверить обновление прошивки ESP32.
 */
int ESPUpdate::checkForUpdate() {
    Log::D(__func__);

    appNewVersion = "";

    if (WiFi.status() != WL_CONNECTED) {
        Log::D("Wi-Fi is not connected.");
        return 0;
    }

    for (int level = 1; level <= 3; ++level) {
        String versionUrl = Core::settings.getVersionURL(level);
        Log::D("Leve: %d, url: %s",level, versionUrl.c_str());
        if (versionUrl.isEmpty()) {
            continue;
        }

        HTTPClient http;
        http.begin(versionUrl + "?r=" + String(random(10000)));
        http.setTimeout(10000);

        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String candidateVersion = http.getString();
            candidateVersion.trim();
            if (candidateVersion.toInt() > atol(APP_VERSION)) {
                appNewVersion = candidateVersion;
                Log::D("Old version %s", APP_VERSION);
                Log::D("New version %s", appNewVersion.c_str());
                Log::D("ESP update level: %d", level);
                http.end();
                return level;
            }
        }

        http.end();
    }

    return 0;
}

String ESPUpdate::getNewAppVersion() {
    return appNewVersion;
}

/**
 * Выполнить обновление прошивки ESP32.
 */
bool ESPUpdate::FirmwareUpdate(int level, ProgressCallback progress) {
    Log::D(__func__);

    HTTPClient http;
    http.begin(Core::settings.getFirmwareURL(level));
    http.setTimeout(10000); // 10 секунд

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        if (contentLength <= 0) {
            Log::D("Invalid content length.");
            http.end();
            return false;
        }

        // Получить MD5-хэш
        String newmd5;
        if (!getMD5Hash(newmd5, level)) {
            Log::D("MD5 checksum not provided by server.");
            http.end();
            return false;
        }

        if (newmd5.isEmpty()) {
            Log::D("MD5 checksum is empty.");
            http.end();
            return false;
        }

        // Найти целевой раздел для обновления
        const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
        if (!update_partition) {
            Log::D("No OTA partition found");
            http.end();
            return false;
        }

        // Инициализировать OTA
        esp_ota_handle_t ota_handle;
        esp_err_t err = esp_ota_begin(update_partition, contentLength, &ota_handle);
        if (err != ESP_OK) {
            Log::D("esp_ota_begin failed: %s", esp_err_to_name(err));
            http.end();
            return false;
        }

        if (contentLength > update_partition->size) {
            Log::D("Firmware too large! Max: %d bytes", update_partition->size);
            esp_ota_abort(ota_handle);
            http.end();
            return false;
        }

        // Начать процесс скачивания и записи
        WiFiClient *stream = http.getStreamPtr();
        MD5Builder md5;
        md5.begin();

        size_t total = 0;
        int lastPercent = -1;
        unsigned long startTime = millis();
        bool timeoutOccurred = false;

        uint8_t buffer[4096];
        Log::L("Старт обновления...");
        if (progress) {
            progress(0);
            lastPercent = 0;
        }

        // Цикл скачивания с таймаутом
        while (total < contentLength && !timeoutOccurred) {
            unsigned long dataWaitStart = millis();
            while (!stream->available() && millis() - dataWaitStart < 120000) {
                delay(100);
                if ((millis() - dataWaitStart) % 5000 == 0) {
                    Log::D("Waiting for data... %d sec", (millis() - dataWaitStart) / 1000);
                }
            }

            if (!stream->available()) {
                timeoutOccurred = true;
                Log::E("Data timeout after %d ms", millis() - dataWaitStart);
                break;
            }

            // Прочитать данные
            int len = stream->read(buffer, sizeof(buffer));
            if (len < 0) {
                Log::E("Read error: %d", len);
                break;
            } else if (len == 0) {
                Log::D("No data, retrying...");
                continue;
            }

            // Записать в OTA
            if (esp_ota_write(ota_handle, buffer, len) != ESP_OK) {
                Log::D("Write failed");
                break;
            }

            md5.add(buffer, len);
            total += len;
            startTime = millis();

            Log::D("Downloaded: %d KB / %d KB", total / 1024, contentLength / 1024);
            if (progress) {
                int percent = static_cast<int>((static_cast<uint64_t>(total) * 100ULL) / static_cast<uint64_t>(contentLength));
                if (percent != lastPercent) {
                    lastPercent = percent;
                    progress(percent);
                }
            }
            //pLoad::getInstance().text("Update: " + String((total * 100) / contentLength) + "%");
        }

        // Проверки после скачивания
        if (total != contentLength || timeoutOccurred) {
            Log::E("Download failed: %d/%d bytes, timeout: %d", total, contentLength, timeoutOccurred);
            esp_ota_abort(ota_handle);
            http.end();
            return false;
        }

        // Проверить MD5
        md5.calculate();
        String downloadedMD5 = md5.toString();
        if (downloadedMD5 != newmd5) {
            Log::E("MD5 mismatch! Expected: %s, got: %s", newmd5.c_str(), downloadedMD5.c_str());
            esp_ota_abort(ota_handle);
            http.end();
            return false;
        }

        // Завершить OTA
        if (esp_ota_end(ota_handle) != ESP_OK) {
            Log::E("esp_ota_end failed");
            http.end();
            return false;
        }

        // Установить новый раздел как загрузочный
        if (esp_ota_set_boot_partition(update_partition) != ESP_OK) {
            Log::E("Failed to set boot partition");
            http.end();
            return false;
        }

        markNewFirmwarePendingValidation();

        // Перезагрузить
        Log::L("Перезагрузка после обновления");
        esp_restart();
    } else {
        Log::E("Failed to download firmware. HTTP code: %d", httpCode);
        http.end();
        return false;
    }

    return true;  // Хотя компилятор не дойдет сюда, но для полноты
}

/**
 * Получить MD5-хэш прошивки.
 */
bool ESPUpdate::getMD5Hash(String& md5, int level) {
    Log::D(__func__);

    // Проверить, подключен ли Wi-Fi
    if (WiFi.status() != WL_CONNECTED) {
        Log::D("Wi-Fi is not connected.");
        return false;
    }

    HTTPClient http;
    http.begin(Core::settings.getHashURL(level) + "?r=" + String(random(10000)));
    http.setTimeout(10000); // 10 секунд

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        md5 = http.getString();
        md5.trim();
        http.end();
        return true;
    } else {
        Log::D("Failed to get hash file. HTTP code: %d", httpCode);
    }

    http.end();
    return false;
}

/**
 * Откат на предыдущую версию.
 */
void ESPUpdate::rollbackUpdate() {
    Log::D("Starting rollback...");
    if (Update.isFinished()) {
        Update.rollBack();
        Log::D("Rollback complete. Rebooting...");
        ESP.restart();
    } else {
        Log::D("Rollback not possible. Firmware is not in a valid state.");
    }
}

/**
 * Переключить OTA-раздел.
 */
void ESPUpdate::switchOTAPartition() {
    const esp_partition_t* current_partition = esp_ota_get_running_partition();
    if (current_partition == nullptr) {
        ESP_LOGE(OTA_TAG, "Failed to get current partition");
        return;
    }

    const esp_partition_t* target_partition = esp_ota_get_next_update_partition(current_partition);
    if (target_partition == nullptr) {
        ESP_LOGE(OTA_TAG, "Target partition not found!");
        return;
    }

    esp_app_desc_t app_desc;
    if (esp_ota_get_partition_description(target_partition, &app_desc) != ESP_OK) {
        ESP_LOGE(OTA_TAG, "Invalid firmware in target partition");
        return;
    }

    ESP_LOGI(OTA_TAG, "Setting boot partition to: %s (address 0x%08x)",
             target_partition->label,
             static_cast<unsigned int>(target_partition->address));

    if (esp_ota_set_boot_partition(target_partition) != ESP_OK) {
        ESP_LOGE(OTA_TAG, "Failed to set new boot partition!");
        return;
    }

    markNewFirmwarePendingValidation();
    ESP_LOGI(OTA_TAG, "Restarting system...");
    esp_restart();
}
