#pragma once

#include <Arduino.h>

class PinTester {
public:
    /**
     * @brief Инициализирует режим тестирования пинов.
     * Переводит все пины, используемые в проекте (ESP32 и MCP), в режим OUTPUT,
     * чтобы можно было управлять их состоянием.
     */
    static void initTestMode();

    /**
     * @brief Устанавливает состояние (HIGH или LOW) для указанного пина.
     * @param device Имя устройства ("ESP32", "MCP0", "MCP1", "MCP2").
     * @param pin Номер пина на устройстве.
     * @param state Желаемое состояние (HIGH или LOW).
     * @return true, если операция прошла успешно, иначе false.
     */
    static bool setPinState(const String& device, int pin, int state);

    /**
     * @brief Читает текущее состояние указанного пина.
     * @param device Имя устройства ("ESP32", "MCP0", "MCP1", "MCP2").
     * @param pin Номер пина на устройстве.
     * @return Состояние пина (HIGH или LOW), или -1 в случае ошибки.
     */
    static int getPinState(const String& device, int pin);
};
