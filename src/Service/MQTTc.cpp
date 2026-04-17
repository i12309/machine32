    #include "MQTTC.h"
    #include "Core.h"
    #include "Licence.h"
    #include "WiFiConfig.h"
    
    // Обработчик входящих сообщений
    void MQTTc::callback(char* topic, byte* payload, unsigned int length) {
        // Создаем строку из байтового массива
        String message = String((char*)payload);

        // Проверяем, не лицензия ли это
        String topicStr = String(topic);
        if (topicStr.endsWith("/licence_push")) {
            Licence::getInstance().loadFromJSON(message.c_str());
            Licence::getInstance().saveToPreferences();
            MQTTc::getInstance().licenceReceived = true;
        }

        // новый конфиг 
        if (topicStr.endsWith("/config_push")) {
            if (FileSystem::saveConfig(message)) ESP.restart();
        }
    }

    String MQTTc::mqttState() {
        switch (client.state()) {
            case 0:  return "Connected";
            case -1: return "WiFi not connected";
            case -4: return "Connection timeout";
            case -5: return "Rejected protocol";
            case -6: return "Bad client ID";
            case -7: return "Unavailable server";
            default: return "Unknown error: " + String(client.state());
        }
    }


    
    // Подключаемся к MQTT брокеру
    void MQTTc::waitForLicence(unsigned long timeoutMs) {
        unsigned long start = millis();
        while (!licenceReceived && (millis() - start < timeoutMs)) {
            client.loop();
            delay(10);
        }
    }

    bool MQTTc::connect() {
        // Параметры подключения (замените на свои)
        const char* mqtt_server = Core::settings.SERVER.c_str();
        int mqtt_port = 1883;
        const char* mqtt_user = "ioESP32";
        const char* mqtt_password = "Harvester32";
        
        // Формируем топик для LWT
        String willTopic = "io32/" + macAddr + "/status";
        String willMessage = "{\"status\": \"offline\"}";
        // Настраиваем клиент
        client.setClient(espClient);
        client.setServer(mqtt_server, mqtt_port);
        client.setCallback(callback); 
        client.setKeepAlive(120);
        // Keep MQTT operations short to avoid watchdog stalls during boot.
        client.setSocketTimeout(2);
        // ПОДКЛЮЧАЕМСЯ С LWT (правильный способ)
        if (client.connect(("esp32_" + macAddr).c_str(),mqtt_user,mqtt_password,willTopic.c_str(),0,true,willMessage.c_str())) {
            licenceReceived = false;
            Serial.println("MQTT - is connected! "+ mqttState());
            
            // Подписываемся

            // лицензия 
            String licenceTopic = "io32/" + macAddr + "/licence_push";
            client.subscribe(licenceTopic.c_str());

            // конфиг  
            String configTopic = "io32/" + macAddr + "/config_push";
            client.subscribe(configTopic.c_str());
            
            delay(100);

            status_send(); // отправить статус 
            device_send(); // отправить инфо об устройстве 
            licence_request(); // запросить лицензию
            waitForLicence(5000);
            config_send(); // отправить конфиг

            return true;
        }

        Serial.println("MQTT - NOT! connected. " + mqttState());
        return false;
    }

    // Обновляем статус и запрашиваем лицензию
    void MQTTc::device_send() {
        if (!client.connected()) return;
        String topic = "io32/" + macAddr + "/device";
        JsonDocument doc;
        doc["firmware"] = APP_VERSION;
        doc["model"] = Core::config.machine;
        doc["Chip"] = ESP.getChipModel();
        doc["Rev"] = ESP.getChipRevision();
        doc["IP"] = WiFi.localIP().toString();
        String payload;
        serializeJson(doc, payload);
        client.publish(topic.c_str(), payload.c_str(), false);
    }
    
    void MQTTc::status_send() {
        if (!client.connected()) return;
        String topic = "io32/" + macAddr + "/status";
        JsonDocument doc;
        doc["status"] = "online";
        doc["RSSI"] = WiFi.RSSI();
        doc["config_version"] = Core::config.configVersion;
        String payload;
        serializeJson(doc, payload);
        client.publish(topic.c_str(), payload.c_str(), false);
    }

    void MQTTc::up_send() {
        if (!client.connected()) return;
        String topic = "io32/" + macAddr + "/status";
        JsonDocument doc;
        doc["status"] = "up";
        String payload;
        serializeJson(doc, payload);
        client.publish(topic.c_str(), payload.c_str(), false);
    }

    void MQTTc::licence_request() {
        if (!client.connected()) return;
        String topic = "io32/" + macAddr + "/licence_pull";
        JsonDocument doc;
        doc["get"] = "licence";
        String payload;
        serializeJson(doc, payload);
        client.publish(topic.c_str(), payload.c_str(), false);
    }

    void MQTTc::config_send() {
        if (!client.connected()) return;
        String topic = "io32/" + macAddr + "/config_pull";
        String payload;
        JsonDocument doc;
        doc["config"] = Core::config.doc;  // Помещаем конфиг в поле "config"
        serializeJson(Core::config.doc, payload);
        client.publish(topic.c_str(), payload.c_str(), false);
    }

    // Отправляем лог
    // Отправляем лог в правильном JSON-формате
    void MQTTc::log(const String& level, const String& message) {
        if (!client.connected()) return;
        String topic = "io32/" + macAddr + "/logs";
        JsonDocument doc;
        doc["level"] = level;
        doc["message"] = message;
        String payload;
        serializeJson(doc, payload);
        client.publish(topic.c_str(), payload.c_str(), false);
    }
    
    // Методы для отправки статистики
    void MQTTc::sendDevice() {
        if (!client.connected()) return;
        String topic = "io32/" + macAddr + "/stats_device";
        JsonDocument doc;
        // Здесь можно реализовать отправку device статистики
        // Пока заглушка
        doc["type"] = "device_stats";
        String payload;
        serializeJson(doc, payload);
        client.publish(topic.c_str(), payload.c_str(), false);
    }

    void MQTTc::sendJob() {
        if (!client.connected()) return;
        String topic = "io32/" + macAddr + "/stats_job";
        JsonDocument doc;
        // Отправка последнего завершённого задания
        doc["type"] = "job_stats";
        String payload;
        serializeJson(doc, payload);
        client.publish(topic.c_str(), payload.c_str(), false);
    }

    void MQTTc::sendOperation() {
        if (!client.connected()) return;
        String topic = "io32/" + macAddr + "/stats_operation";
        JsonDocument doc;
        // Отправка всех operation
        doc["type"] = "operation_stats";
        String payload;
        serializeJson(doc, payload);
        client.publish(topic.c_str(), payload.c_str(), false);
    }

    // Вызывать в основном loop()
    void MQTTc::process() {
        if (getInstance().client.connected()) {
            getInstance().client.loop();

            // Периодически отправляем ping для поддержания соединения
            unsigned long now = millis();
            if (now - getInstance().lastPingTime > 30000) { // каждые 30 секунд
                getInstance().lastPingTime = now;
                getInstance().up_send(); // отправляем статус как ping
            }
        } else
        {
            if (WiFiConfig::getInstance().isConnect()){

                // Периодически пробуем переподключиться
                unsigned long now = millis();
                if (now - getInstance().lastPingTime > 300000) { // каждые 300 секунд
                    getInstance().lastPingTime = now;
                    getInstance().connect();
                }
            }
        }
    }
