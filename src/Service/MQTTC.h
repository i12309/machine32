#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

class MQTTc {
private:
    // Приватный конструктор (синглтон)
    MQTTc() {
        // Получаем MAC-адрес и форматируем
        macAddr = WiFi.macAddress();
        macAddr.replace(":", "");
        licenceReceived = false;
        client.setBufferSize(2048);
    }

    // Обработчик входящих сообщений
    static void callback(char* topic, byte* payload, unsigned int length);

    // Вспомогательная функция для получения текстового состояния
    String mqttState();
    void waitForLicence(unsigned long timeoutMs);

    // Объекты MQTT
    WiFiClient espClient;
    PubSubClient client;
    bool licenceReceived;
    String macAddr;
    long lastPingTime;

public:
    // Получаем единственный экземпляр (синглтон)
    static MQTTc& getInstance() {
        static MQTTc instance;
        return instance;
    }

    // Подключаемся к MQTT брокеру
    bool connect();

    // Методы публикации сообщений
    void device_send();
    void status_send();
    void up_send();
    void licence_request();
    void config_send();
    void log(const String& level, const String& message);

    // Методы отправки статистики
    void sendDevice();
    void sendJob();
    void sendOperation();

    // Обновление цикла (вызывать в loop)
    static void process();
};