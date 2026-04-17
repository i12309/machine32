#include "HServer.h"
#include "Data.h"

// Реализация Singleton
HServer& HServer::getInstance() {
    static HServer instance;
    return instance;
}

// Конструктор
HServer::HServer()
    : server(80), _wifiConfig(WiFiConfig::getInstance()) {
    _username = "123";
    _password = "123";
    _isWork = false;
}

// Запуск сервера
void HServer::begin() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/web", HTTP_GET, [this]() { handleWebRedirect(); });
    server.on("/web-ui-settings", HTTP_GET, [this]() { handleWebUiConfigGet(); });

    server.on("/config", HTTP_GET, [this]() { handleConfigGet(); });
    server.on("/config", HTTP_POST, [this]() { handleConfigPost(); });
    server.on("/data", HTTP_GET, [this]() { handleDataGet(); });
    server.on("/data", HTTP_POST, [this]() { handleDataPost(); });
    server.on("/stats", HTTP_GET, [this]() { handleStatsGet(); });

    server.on("/ota-esp32", HTTP_GET, [this]() { handleOtaESP32(); });
    server.on("/ota-esp32-post", HTTP_POST,
        [this]() { handleOtaESP32post_1(); },
        [this]() { handleOtaESP32post_2(); });

    server.begin();
    _isWork = true;
    Log::L("HTTP server started.");
}

// Обработка клиентов
void HServer::process()
{
    if (!getInstance()._isWork) return;
    const bool staConnected = getInstance()._wifiConfig.isConnect();
    const bool apActive = (WiFi.getMode() & WIFI_MODE_AP) != 0;
    if (staConnected || apActive) getInstance().server.handleClient();
}

// Проверка аутентификации
bool HServer::isAuthenticated() {
    if (server.hasHeader("Authorization")) {
        String authHeader = server.header("Authorization");
        if (authHeader.startsWith("Basic ")) {
            String encoded = authHeader.substring(6);
            String decoded = base64Decode(encoded);
            String expected = String(_username) + ":" + String(_password);
            if (decoded == expected) {
                return true;
            }
        }
    }

    server.sendHeader("WWW-Authenticate", "Basic realm=\"Restricted Area\"");
    server.send(401, "text/plain", "Unauthorized");
    return false;
}

// Обработчик корневой страницы
void HServer::handleRoot() {
    if (!isAuthenticated()) return;

    String html = R"(
<!DOCTYPE html><html><head><meta charset='UTF-8'><style></style></head><body>
    <a href='/config'>Открыть config.json</a>
    <br><br>
    <a href='/data'>Открыть data.json</a>
    <br><br>
    <a href='/stats'>Открыть stats.json</a>
    <br><br>
    <a href='/ota-esp32'>Обновить программу</a>
    <br><br>
    <br><br>
    <a href='/web'>WEB</a>
</body></html>
)";
    server.send(200, "text/html", html);
}

// Обработчик получения config.json
void HServer::handleWebRedirect() {
    if (!isAuthenticated()) return;

    const IPAddress staIp = WiFi.localIP();
    const IPAddress apIp = WiFi.softAPIP();
    const bool hasStaIp = staIp != IPAddress(static_cast<uint32_t>(0));
    const bool hasApIp = apIp != IPAddress(static_cast<uint32_t>(0));
    const String deviceIp = hasStaIp ? staIp.toString() : (hasApIp ? apIp.toString() : "0.0.0.0");

    const String webUrl =
        "http://" + Core::settings.SERVER + "/" +
        Core::config.machine + "/" +
        Core::config.group + "/" +
        Core::config.name + "/web.html?ip=" + deviceIp;

    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Location", webUrl, true);
    server.send(302, "text/plain; charset=utf-8", "Redirect to WEB UI");
}

void HServer::handleWebUiConfigGet() {
    JsonDocument doc;
    doc["log"] = Core::settings.log;

    String payload;
    serializeJson(doc, payload);

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    server.send(200, "application/json; charset=utf-8", payload);
}

void HServer::handleConfigGet() {
    if (!isAuthenticated()) return;

    String formattedJson;
    if (!FileSystem::readConfig(formattedJson)) {
        server.send(500, "text/plain charset=utf-8", "Failed to read config");
        return;
    }

    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <style>textarea{width:500px;height:300px}</style>
</head>
<body>
    <h2>config.json</h2>
    <form method="POST" action="/config" accept-charset='UTF-8'>
        <textarea name="data" rows="30" cols="50">)"
        + formattedJson + R"(</textarea><br><br>
        <input type="submit" value="Submit">
    </form>
</body>
</html>
)";
    server.send(200, "text/html charset=utf-8", html);
}

// Обработчик сохранения config.json
void HServer::handleConfigPost() {
    if (!isAuthenticated()) return;

    if (server.hasArg("data")) {
        String data = server.arg("data");
        if (FileSystem::saveConfig(data)) {
            server.send(200, "text/html charset=utf-8",
                "<p>Config saved successfully!</p>"
                "<a href='/config'>Back to editor</a>");
            Core::config.load();
            Data::tuning.load();
            if (Core::data.load()) {
                Data::profiles.load();
                Data::tasks.load();
            }
        } else {
            server.send(500, "text/plain charset=utf-8", "Failed to save config");
        }
    } else {
        server.send(400, "text/plain charset=utf-8", "No data received");
    }
}

// Обработчик получения stats.json

// Data.json GET
void HServer::handleDataGet() {
    if (!isAuthenticated()) return;

    if (!Core::data.load()) {
        server.send(500, "text/plain charset=utf-8", "Failed to load data");
        return;
    }

    String formattedJson;
    if (!FileSystem::readData(formattedJson)) {
        serializeJsonPretty(Core::data.doc, formattedJson);
        if (formattedJson.isEmpty()) {
            formattedJson = "{}";
        }
    }

    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <style>textarea{width:500px;height:300px}</style>
</head>
<body>
    <h2>data.json</h2>
    <form method="POST" action="/data" accept-charset='UTF-8'>
        <textarea name="data" rows="30" cols="50">)"
        + formattedJson + R"(</textarea><br><br>
        <input type="submit" value="Submit">
    </form>
</body>
</html>
)";
    server.send(200, "text/html charset=utf-8", html);
}

// Data.json POST
void HServer::handleDataPost() {
    if (!isAuthenticated()) return;

    if (server.hasArg("data")) {
        String data = server.arg("data");
        if (FileSystem::saveData(data)) {
            server.send(200, "text/html charset=utf-8",
                "<p>Data saved successfully!</p>"
                "<a href='/data'>Back to editor</a>");
            Core::data.load();
            Data::profiles.load();
            Data::tasks.load();
        } else {
            server.send(500, "text/plain charset=utf-8", "Failed to save data");
        }
    } else {
        server.send(400, "text/plain charset=utf-8", "No data received");
    }
}
void HServer::handleStatsGet() {
    if (!isAuthenticated()) return;

    if (!LittleFS.exists("/stats.json")) {
        server.send(404, "text/plain charset=utf-8", "stats.json not found");
        return;
    }

    File file = LittleFS.open("/stats.json", "r");
    if (!file) {
        server.send(500, "text/plain charset=utf-8", "Failed to read stats.json");
        return;
    }

    String content;
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
        server.send(500, "text/plain charset=utf-8", "Failed to parse stats.json");
        return;
    }
    serializeJsonPretty(doc, content);

    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <style>textarea{width:500px;height:300px}</style>
</head>
<body>
    <h2>stats.json</h2>
    <textarea readonly rows="30" cols="50">)"
        + content + R"(</textarea>
</body>
</html>
)";
    server.send(200, "text/html charset=utf-8", html);
}

// Обработчик OTA (GET)
void HServer::handleOtaESP32() {
    if (!isAuthenticated()) return;
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", otaForm);
}

// Обработчик OTA (часть 1)
void HServer::handleOtaESP32post_1() {
    Log::D(__func__);
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
}

// Обработчик OTA (часть 2)
void HServer::handleOtaESP32post_2() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        pWAIT::wait("", "Обновление..", "", 0, nullptr, false);
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            ESPUpdate::getInstance().markNewFirmwarePendingValidation();
        } else {
            Update.printError(Serial);
        }
    }
}

// Вспомогательная функция: индекс символа
int HServer::indexOfChar(const char* str, char c) {
    const char* ptr = strchr(str, c);
    if (ptr) {
        return ptr - str;
    }
    return -1;
}

// Декодирование Base64
String HServer::base64Decode(String input) {
    const char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String output = "";
    int i = 0;
    int len = input.length();
    while (i < len) {
        uint32_t sextetA = input[i] == '=' ? 0 & i++ : indexOfChar(base64Chars, input[i++]);
        uint32_t sextetB = input[i] == '=' ? 0 & i++ : indexOfChar(base64Chars, input[i++]);
        uint32_t sextetC = input[i] == '=' ? 0 & i++ : indexOfChar(base64Chars, input[i++]);
        uint32_t sextetD = input[i] == '=' ? 0 & i++ : indexOfChar(base64Chars, input[i++]);
        uint32_t triple = (sextetA << 18) + (sextetB << 12) + (sextetC << 6) + sextetD;
        output += char((triple >> 16) & 0xFF);
        if (input[i - 2] != '=') output += char((triple >> 8) & 0xFF);
        if (input[i - 1] != '=') output += char(triple & 0xFF);
    }
    return output;
}
