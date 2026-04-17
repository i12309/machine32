#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "NexHardware.h"

#include "config.h"
#include "Core.h"
#include "Log.h"
#include "UI/Main/pLoad.h"

/**
 * Код загрузки прошивки Nextion согласно протоколу v1.1
 * Основано на официальной документации: https://nextion.tech/2017/12/08/nextion-hmi-upload-protocol-v1-1/
 */

class NexUpdate {
public:
    static NexUpdate& getInstance() {
        static NexUpdate instance;
        return instance;
    }

    NexUpdate() {
        _download_baudrate = 921600;
        _nextionAddress = 0;
        _addressMode = false;
        _totalSize = 0;
        _downloaded = 0;
        _currentVersion = -1;
        _updateAvailable = false;
    }

    /**
     * Проверить наличие новой версии прошивки Nextion.
     * @return 0 если обновления нет, иначе уровень (1..3), где найдено обновление.
     */
    int checkForUpdate() {
        return _performUpdateCheck(_currentVersion);
    }

    String getNewAppVersion() const {
        return _appNewVersion;
    }

    int getCurrentVersion() const {
        return _currentVersion;
    }

    void setCurrentVersion(int vTFT) {
        _currentVersion = vTFT;
    }

    bool upload(int level = 2, bool reboot = false) {
        Serial.println("=== Nextion Upload Protocol v1.1 ===");
        
        if (!_startHTTPConnection(level)) {
            Serial.println("✗ HTTP connection failed");
            return false;
        }
        
        uint32_t baudrate = _getBaudrate();
        if (baudrate == 0) {
            Serial.println("✗ Baudrate detection failed");
            return false;
        }
        
        if (!_setDownloadBaudrate(_download_baudrate)) {
            Serial.println("✗ Download mode activation failed");
            return false;
        }
        
        if (!_downloadTft()) {
            Serial.println("✗ Download failed");
            return false;
        }

        Serial.println("✓ Upload completed successfully!");
        if (reboot) {// Перезагрузка для применения обновления
            esp_restart();
        }
        return true;
    }
private:
    // Основные перемены класса
    uint32_t _download_baudrate;
    HTTPClient http;
    WiFiClient* stream;
    size_t _totalSize;        // Размер файла прошивки
    size_t _downloaded;       // Количество скачанных байт
    int _currentVersion;
    bool _updateAvailable;
    String _appNewVersion;

    // Переменные протокола Nextion
    uint16_t _nextionAddress = 0; // Адрес дисплея из строки подключения
    bool _addressMode = false;    // Режим адресации активен если адрес != 0

    int _performUpdateCheck(int vTFT) {
        Log::D(__func__);

        _appNewVersion = "";
        _updateAvailable = false;

        if (WiFi.status() != WL_CONNECTED) {
            Log::L("Wi-Fi is not connected.");
            return 0;
        }

        for (int level = 1; level <= 3; ++level) {
            String versionUrl = Core::settings.getTFTVersionURL(level);
            if (versionUrl.isEmpty()) {
                continue;
            }

            HTTPClient versionHttp;
            versionHttp.begin(versionUrl + "?r=" + String(random(10000)));
            versionHttp.setTimeout(10000);

            int httpCode = versionHttp.GET();
            if (httpCode == HTTP_CODE_OK) {
                String candidateVersion = versionHttp.getString();
                candidateVersion.trim();
                int newVersion = candidateVersion.toInt();
                if (newVersion > vTFT) {
                    _appNewVersion = candidateVersion;
                    Log::L("New firmware available for Nextion");
                    Log::D("Old Nextion version %d", vTFT);
                    Log::D("New Nextion version %d", newVersion);
                    Log::D("TFT update level: %d", level);
                    _updateAvailable = true;
                    versionHttp.end();
                    return level;
                }
            } else {
                Log::E("Nextion firmware check error. HTTP code: %d", httpCode);
            }

            versionHttp.end();
        }

        return 0;
    }

    // Метод установки HTTP соединения
    bool _startHTTPConnection(int level) {
        Serial.println("Starting HTTP connection...");

        String firmwareUrl = Core::settings.getTFTFirmwareURL(level);
        if (firmwareUrl.isEmpty()) {
            Serial.println("Firmware URL is empty");
            return false;
        }

        http.begin(firmwareUrl);
        http.setTimeout(30000); // 30 секунд таймаут
        
        int httpCode = http.GET();
        if (httpCode != HTTP_CODE_OK) {
            Serial.println(firmwareUrl);
            Serial.print("HTTP error code: "); 
            Serial.println(httpCode);
            http.end();
            return false;
        }
        
        _totalSize = http.getSize();
        if(_totalSize <= 0) {
            Serial.println("Invalid file size");
            http.end();
            return false;
        }
        
        Serial.print("File size: "); 
        Serial.println(_totalSize);
        stream = http.getStreamPtr();
        return true;
    }
    
    // Правильная последовательность установки соединения согласно протоколу
    bool _establishConnection(uint32_t baudrate) {
        Serial.print("Testing baudrate: ");
        Serial.println(baudrate);
    
        nexSerial.begin(baudrate);
        
        // Отправляем пустую команду для очистки
        nexSerial.write(0xFF); 
        nexSerial.write(0xFF); 
        nexSerial.write(0xFF);
        delay(50);
        
        // 1. Команда выхода из режима повторной обработки
        String exitCmd = "DRAKJHSUYDGBNCJHGJKSHBDN";
        nexSerial.print(exitCmd);
        nexSerial.write(0xFF); nexSerial.write(0xFF); nexSerial.write(0xFF);
        
        // 2. Стандартная команда подключения
        nexSerial.print("connect");
        nexSerial.write(0xFF); nexSerial.write(0xFF); nexSerial.write(0xFF);
        
        // 3. Команда подключения с широковещательным адресом
        nexSerial.write(0xFF); nexSerial.write(0xFF); // Широковещательный адрес 0xFFFF
        nexSerial.print("connect");
        nexSerial.write(0xFF); nexSerial.write(0xFF); nexSerial.write(0xFF);
        
        // ПРАВИЛЬНАЯ задержка согласно протоколу: 1000000/baudrate + 30 мс
        uint32_t protocolDelay = (1000000 / baudrate) + 30;
        delay(protocolDelay);
    
        // Чтение ответа
        String response = "";
        unsigned long start = millis();
        while(millis() - start < 2000) { // Увеличиваем таймаут
            if(nexSerial.available()) {
                char c = nexSerial.read();
                response += c;
                // Ищем окончание строки подключения
                if(response.endsWith("\xFF\xFF\xFF") && response.indexOf("comok") != -1) {
                    break;
                }
            }
        }
    
        // Парсим строку подключения
        if(response.indexOf("comok") != -1) {
            Serial.println("✓ Connection established");
            Serial.print("Response: "); Serial.println(response);
            
            // Извлекаем адрес из строки подключения
            _parseConnectionString(response);
            return true;
        }
        
        return false;
    }
    
    // Парсинг строки подключения для извлечения адреса
    void _parseConnectionString(const String& response) {
        // Пример: comok 1,38024-2556,NX4024T032_011R,99,61488,D264B8204F0E1828,16777216
        int dashIndex = response.indexOf('-');
        int commaIndex = response.indexOf(',', dashIndex);
        
        if(dashIndex != -1 && commaIndex != -1) {
            String addressStr = response.substring(dashIndex + 1, commaIndex);
            _nextionAddress = addressStr.toInt();
            _addressMode = (_nextionAddress != 0);
            
            Serial.print("Nextion address: "); 
            Serial.print(_nextionAddress);
            Serial.print(" (0x");
            Serial.print(_nextionAddress, HEX);
            Serial.println(")");
            
            if(_addressMode) {
                Serial.println("⚠ Address mode is ACTIVE - all commands will use address prefix");
            }
        }
    }
    
    uint32_t _getBaudrate() {
        // Начинаем с самой маленькой скорости согласно протоколу
        uint32_t baudrates[] = {2400, 4800, 9600, 19200, 38400, 57600, 115200};
        
        for(uint8_t i = 0; i < sizeof(baudrates)/sizeof(baudrates[0]); i++) {
            if(_establishConnection(baudrates[i])) {
                return baudrates[i];
            }
        }
        Serial.println("✗ No valid baudrate found");
        return 0;
    }

    bool _setDownloadBaudrate(uint32_t baudrate) {
        Serial.println("Setting download baudrate...");

        // Очищаем буферы
        while(nexSerial.available()) nexSerial.read();
        
        String filesize_str = String(_totalSize, 10);
        String baudrate_str = String(baudrate, 10);
        // Протокол: whmi-wri filesize,baud,res0
        String cmd = "whmi-wri " + filesize_str + "," + baudrate_str + ",0";
        
        Serial.print("Command: "); Serial.println(cmd);
        
        // Отправляем команду с учетом адресного режима
        _sendCommand(cmd.c_str());
        delay(100);
        
        // Переключаем скорость ПОСЛЕ отправки команды
        nexSerial.begin(baudrate);
        delay(100); // Даем время на переключение
        
        // Ждем подтверждение 0x05 в течение 500мс согласно протоколу
        unsigned long start = millis();
        while(millis() - start < 500) {
            if(nexSerial.available()) {
                uint8_t response = nexSerial.read();
                Serial.print("Response: 0x"); Serial.println(response, HEX);
                if(response == 0x05) {
                    Serial.println("✓ Download mode activated");
                    return true;
                }
            }
        }
        
        Serial.println("✗ Failed to activate download mode");
        return false;
    }

    // Правильная отправка команд с учетом адресного режима
    void _sendCommand(const char* cmd) {
        // Если активен адресный режим, добавляем префикс
        if(_addressMode) {
            // Байты адреса в обратном порядке (little-endian)
            uint8_t addr_low = _nextionAddress & 0xFF;
            uint8_t addr_high = (_nextionAddress >> 8) & 0xFF;
            nexSerial.write(addr_low);
            nexSerial.write(addr_high);
        }
        
        nexSerial.print(cmd);
        nexSerial.write(0xFF);
        nexSerial.write(0xFF);
        nexSerial.write(0xFF);
    }

    bool _downloadTft(void) {
        uint8_t buffer[4096];
        _downloaded = 0;
        uint32_t blockCounter = 0;

        Serial.println("Starting TFT download...");
        Serial.printf("Total size: %d bytes\n", _totalSize);

        while(_downloaded < _totalSize) {
            // Размер текущего блока
            size_t blockSize = min((size_t)4096, _totalSize - _downloaded);
            
            // Читаем блок данных
            size_t bytesRead = stream->readBytes(buffer, blockSize);
            if(bytesRead != blockSize) {
                Serial.printf("✗ Read error: expected %d, got %d\n", blockSize, bytesRead);
                return false;
            }
            
            // Отправляем блок с учетом адресного режима
            if(_addressMode) {
                uint8_t addr_low = _nextionAddress & 0xFF;
                uint8_t addr_high = (_nextionAddress >> 8) & 0xFF;
                nexSerial.write(addr_low);
                nexSerial.write(addr_high);
            }
            
            nexSerial.write(buffer, bytesRead);
            _downloaded += bytesRead;
            blockCounter++;
            
            // Логирование прогресса
            if(blockCounter % 10 == 0) {
                Serial.printf("Block %d: %d/%d bytes (%.1f%%)\n", 
                             blockCounter, _downloaded, _totalSize, 
                             (_downloaded * 100.0) / _totalSize);
            }

            // ОБЯЗАТЕЛЬНО ждем подтверждение 0x05 после каждого блока
            if(!_waitForAck(2000)) {
                Serial.printf("✗ ACK timeout for block %d\n", blockCounter);
                return false;
            }
        }

        Serial.println("✓ All blocks sent successfully");
        
        // Отправляем финальную последовательность
        // Согласно протоколу после последнего блока также должен прийти 0x05
        // Затем Nextion перезагружается и отправляет 0x88
        
        Serial.println("Waiting for Nextion restart and confirmation...");

        unsigned long start = millis();
        /*
        // Ждем финальное подтверждение 0x88
        while (millis() - start < 5000) { // если экран отвечает на другой скорости, тут 0x88 можем не увидеть
            if (nexSerial.available()) {
                uint8_t response = nexSerial.read();
                Serial.printf("Final response: 0x%02X\n", response);
                
                if (response == 0x88) {
                    Serial.println("✓ Nextion restart completed successfully!");
                    delay(500);
                    return true;
                }
            }
            delay(100);
        }

        Serial.println("0x88 not received. Waiting until Nextion starts responding...");
        */
        nexSerial.begin(115200);
        while (millis() - start < 10000) {
            delay(500);
            if (nexInit()) return true;
        }
        
        Serial.println("✗ Nextion did not respond after upload");
        return false;
    }

    bool _waitForAck(uint32_t timeout = 2000) {
        unsigned long start = millis();
        while(millis() - start < timeout) {
            if(nexSerial.available()) {
                uint8_t response = nexSerial.read();
                if(response == 0x05) {
                    return true; // ACK получен
                }
                // Логируем другие коды ответов
                Serial.printf("Unexpected response: 0x%02X\n", response);
            }
            delay(10);
        }
        return false;
    }

};

