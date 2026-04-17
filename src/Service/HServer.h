#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include <base64.h>

#include "ESPUpdate.h"
#include "Core.h"
#include "WiFiConfig.h"
#include "UI/Main/pWAIT.h"

class HServer {
public:
    // Получение единственного экземпляра (Singleton)
    static HServer& getInstance();

    // Конструктор
    HServer();

    // Запуск сервера
    void begin();

    // Обработка клиентов
    static void process();

private:
    WebServer server;
    const char* _username;
    const char* _password;
    WiFiConfig& _wifiConfig;
    bool _isWork;

    // Проверка аутентификации
    bool isAuthenticated();

    // Обработчики маршрутов
    void handleRoot();
    void handleWebRedirect();
    void handleWebUiConfigGet();
    void handleConfigGet();
    void handleConfigPost();
    void handleDataGet();
    void handleDataPost();
    void handleStatsGet();
    void handleOtaESP32();
    void handleOtaESP32post_1();
    void handleOtaESP32post_2();

    // Вспомогательные функции
    String base64Decode(String input);
    int indexOfChar(const char* str, char c);

    // Форма OTA-обновления
    const char* otaForm = R"rawtext(
        <!DOCTYPE html>
        <html>
        <head><meta charset="utf-8"><title>OTA</title></head>
        <body>
        <h2>Обновить прошивку</h2>
        <form id="upload_form">
            <input type="file" name="update" required>
            <input type="submit" value="Update">
        </form>
        <div id="prg">progress: 0%</div>

        <script>
        const form = document.getElementById('upload_form');
        const prg = document.getElementById('prg');

        form.addEventListener('submit', function(e) {
        e.preventDefault();
        const fileInput = form.querySelector('input[type=file]');
        if (!fileInput.files.length) return;
        const file = fileInput.files[0];
        const fd = new FormData();
        fd.append('update', file, file.name);

        const xhr = new XMLHttpRequest();
        xhr.open('POST', '/ota-esp32-post', true);

        xhr.upload.onprogress = function(evt) {
            if (evt.lengthComputable) {
            const percent = Math.round(evt.loaded / evt.total * 100);
            prg.textContent = 'progress: ' + percent + '%';
            if (percent === 100) {
                prg.textContent = 'Загрузка завершена, перезагрузка...';
            }
            }
        };

        xhr.onload = function() {
            if (xhr.status >= 200 && xhr.status < 300) {
            // можно ждать перезагрузку устройства
            setTimeout(()=> { window.location.href = '/'; }, 1000);
            } else {
            prg.textContent = 'Ошибка: ' + xhr.status;
            }
        };

        xhr.onerror = function() { prg.textContent = 'Network error'; };
        xhr.send(fd);
        });
        </script>
        </body>
        </html>
        )rawtext";
};
