#include "Trigger.h"
#include "Registry.h"
#include "Machine/Machine.h"
#include "Data.h"
#include "State/Scene.h"

Trigger* Trigger::instance = nullptr;

void taskTrigger(void *pvParameters) {
    while (true) {
        Trigger::getInstance().process();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Trigger::init(){
        // Создаем задачу на втором ядре
    xTaskCreatePinnedToCore(
            taskTrigger,              // Функция задачи
            "Trigger",            // Имя задачи
            4096,                // Размер стека
            NULL,                // Параметры
            configMAX_PRIORITIES - 1, // максимальный приоритет
            NULL,                // Дескриптор задачи
            0                    // Номер ядра: 0 или 1
    );
}

void Trigger::registerTrigger(){
    // Temporary: keep only TABLE_WORK_LIMIT here.
    // TABLE_HOME / GUILLOTINE_HOME / THROW_FORCE moved to McpTrigger.
    IMachineContext& ctx = Machine::getInstance().context();

    registerSensorTrigger("TABLE_WORK_LIMIT","TABLE","TABLE",-1,0,ctx.mTable->getWorkMove(),
        []() {
            Machine::getInstance().context().mTable->forceStop();
        }
    );

    // Периодическая проверка таймаута мотора стола.
    // Вызывается каждые 10 мс (цикл taskTrigger) пока триггер активен.
    // Вся логика проверки и остановки по таймауту инкапсулирована в Scene::tableStop(NotStop):
    // если сработал TABLE_NOT_UP или TABLE_NOT_DOWN — мотор будет остановлен и ошибка записана.
    registerMotorTimeoutTrigger("TABLE_TIMEOUT", "TABLE",
        []() {
            Scene::getInstance().tableStop(Catalog::StopMode::NotStop);
        }
    );

    // Периодическая проверка таймаута мотора гильотины.
    // Вызывается каждые 10 мс (цикл taskTrigger) пока триггер активен.
    // если сработал GUILLOTINE_NOT_FORWARD или GUILLOTINE_NOT_BACK — мотор остановлен и ошибка записана.
    registerMotorTimeoutTrigger("GUILLOTINE_TIMEOUT", "GUILLOTINE",
        []() {
            Scene::getInstance().guillotineStop(Catalog::StopMode::NotStop);
        }
    );
}
