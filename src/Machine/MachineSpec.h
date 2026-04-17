#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

#include "Catalog.h"

class Registry;

// Каркас спецификации станка.
// Задача класса: хранить эталонный состав device для конкретного MachineType,
// уметь валидировать конфиг и проверять факт создания объектов после загрузки.
class MachineSpec {
public:
    // Описание одного обязательного элемента в секции device.
    struct Requirement {
        String name;             // Имя элемента, например "PAPER" или "MCP0".
        bool criticalForMotion;  // Если true и элемент отсутствует - движение моторами запрещаем.
    };

    // Итог проверки.
    // errors - критичные расхождения, warnings - некритичные замечания.
    struct Report {
        std::vector<String> errors;
        std::vector<String> warnings;
        bool allowMotion = true; // В false переводим, если отсутствует критичный для движения узел.

        bool hasErrors() const { return !errors.empty(); }
        bool hasWarnings() const { return !warnings.empty(); }
    };

    // Создать спецификацию для выбранного типа станка.
    // Объект возвращается по значению и обычно используется один раз на этапе загрузки.
    static MachineSpec get(Catalog::MachineType type);

    Catalog::MachineType type() const { return type_; }

    // Построить дефолтный блок device по спецификации станка.
    // Каркасно: заполняем только структуру и базовые поля, без тонкой настройки всех параметров.
    void fillDeviceDefaults(JsonObject device) const;

    // Проверка структуры и обязательных элементов внутри device из config.json.
    // Проверяем именно наличие секций/элементов. Глубокую валидацию атрибутов добавим следующим шагом.
    Report validateDeviceConfig(JsonObjectConst device) const;

    // Проверка того, что объекты действительно созданы после инициализации в Registry.
    Report validateRegistry(const Registry& registry) const;

private:
    Catalog::MachineType type_ = Catalog::MachineType::UNKNOWN;

    std::vector<Requirement> i2c_;
    std::vector<Requirement> motors_;
    std::vector<Requirement> sensors_;
    std::vector<Requirement> optical_;
    std::vector<Requirement> clutchs_;
    std::vector<Requirement> switchs_;
    std::vector<Requirement> buttons_;
    std::vector<Requirement> encoders_;

    static MachineSpec makeUnknown();
    static MachineSpec makeA();

    void fillDefaultsA(JsonObject device) const;

    static void validateSectionConfig(
        const char* sectionName,
        JsonObjectConst sectionObj,
        const std::vector<Requirement>& required,
        Report& report
    );
};

