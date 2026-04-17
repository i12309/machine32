#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <vector>
#include <map>

#include "Core.h"
#include "Service/Log.h"

class Stats {
private:
    // Синглтон
    Stats() {}
    static Stats* instance;

    // Флаг включения статистики
    bool enabled = false;

    // Путь к файлу
    const char* STATS_PATH = "/stats.json";

    // Структура данных в RAM
    struct DeviceStats {
        uint32_t power_on = 0;
        std::map<String, uint64_t> steps; // мотор -> суммарные шаги
    };

    struct ProfileStats {
        long taskId;
        long profileId;
        uint32_t runs = 0;
        uint64_t products = 0;
        uint64_t paper_um = 0; // метраж бумаги в микрометрах
        uint64_t sheets = 0;
        uint64_t rejects = 0;
        std::map<String, uint64_t> steps; // мотор -> шаги в профиле

        ProfileStats(long tId, long pId) : taskId(tId), profileId(pId) {}
    };

    struct TaskStats {
        long id;
        std::vector<ProfileStats> profiles;

        TaskStats(long taskId) : id(taskId) {}
    };

    DeviceStats device;
    std::vector<TaskStats> tasks;

    // Текущий активный job
    long currentTaskId = -1;
    long currentProfileId = -1;
    float currentRatio = 0.0f;
    bool jobActive = false;

    // Загрузка из JSON
    bool loadFromFile();

    // Сохранение в JSON
    bool saveToFile();

    // Поиск или создание TaskStats
    TaskStats* findOrCreateTask(long taskId);

    // Поиск ProfileStats в TaskStats
    ProfileStats* findOrCreateProfile(TaskStats* task, long profileId);

public:
    // Получить экземпляр
    static Stats& getInstance() {
        if (!instance) {
            instance = new Stats();
        }
        return *instance;
    }

    // Инициализация (вызывать при старте)
    void init();

    // Методы учёта
    void jobStart(long taskId, long profileId, float ratioStepsPerMm);
    void jobFinish();
    void onMotorSteps(const String& motorName, long steps);
    void onSheet();
    void onProduct();
    void onReject();

    // Сохранение
    void save();

    struct MotorStat {
        String name;
        uint64_t steps = 0;
    };

    struct TaskRunStat {
        long id = -1;
        uint64_t runs = 0;
    };

    struct ProfileRunStat {
        long id = -1;
        uint64_t runs = 0;
    };

    struct AggregateStat {
        uint64_t runs = 0;
        uint64_t products = 0;
        uint64_t paper_um = 0;
        uint64_t sheets = 0;
        uint64_t rejects = 0;
        uint32_t profiles = 0;
        std::vector<MotorStat> motors;
    };

    std::vector<MotorStat> getDeviceMotors() const;
    std::vector<TaskRunStat> getTaskRuns() const;
    std::vector<ProfileRunStat> getProfileRuns() const;
    std::vector<ProfileRunStat> getTaskProfileRuns(long taskId) const;

    bool getTaskAggregate(long taskId, AggregateStat& out) const;
    bool getTaskProfileAggregate(long taskId, long profileId, AggregateStat& out) const;
    bool getProfileAggregate(long profileId, AggregateStat& out) const;

    // Отладка
    void printState();
    void printSnapshot();
    void printFsUsage();

    // Полное удаление статистики (RAM + файл).
    bool clearAll();
};
