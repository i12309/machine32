#include "Stats.h"

#include <algorithm>

namespace {
    void resetAggregate(Stats::AggregateStat& out) {
        out.runs = 0;
        out.products = 0;
        out.paper_um = 0;
        out.sheets = 0;
        out.rejects = 0;
        out.profiles = 0;
        out.motors.clear();
    }

    void addSteps(std::map<String, uint64_t>& dst, const std::map<String, uint64_t>& src) {
        for (const auto& kv : src) {
            dst[kv.first] += kv.second;
        }
    }

    std::vector<Stats::MotorStat> mapToMotors(const std::map<String, uint64_t>& steps) {
        std::vector<Stats::MotorStat> result;
        result.reserve(steps.size());
        for (const auto& kv : steps) {
            Stats::MotorStat item;
            item.name = kv.first;
            item.steps = kv.second;
            result.push_back(item);
        }
        return result;
    }
}

// Инициализация статической переменной
Stats* Stats::instance = nullptr;

// Инициализация
void Stats::init() {
    enabled = Core::settings.metrics;
    if (!enabled) {
        Log::D("Stats: disabled");
        return;
    }
    Log::D("Stats: enabled");
    if (!loadFromFile()) Log::D("Stats: failed to load from file, starting fresh");
    device.power_on++; // увеличиваем при каждом включении
    saveToFile();
}

// Загрузка из JSON
bool Stats::loadFromFile() {
    if (!LittleFS.exists(STATS_PATH))  return false;

    File file = LittleFS.open(STATS_PATH, "r");
    if (!file) {
        Log::D("Stats: cannot open file for read");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Log::D("Stats: JSON parse error: %s", error.c_str());
        return false;
    }

    tasks.clear();
    device.steps.clear();

    // Загрузка device
    if (doc["device"].is<JsonObject>()) {
        JsonObject devObj = doc["device"];
        device.power_on = devObj["power_on"] | 0;
        if (devObj["steps"].is<JsonObject>()) {
            JsonObject stepsObj = devObj["steps"];
            for (JsonPair kv : stepsObj) {
                device.steps[String(kv.key().c_str())] = kv.value().as<uint64_t>();
            }
        }
    }

    // Загрузка operation
    if (doc["operation"].is<JsonObject>() && doc["operation"]["tasks"].is<JsonArray>()) {
        JsonArray tasksArr = doc["operation"]["tasks"];
        for (JsonObject taskObj : tasksArr) {
            long taskId = taskObj["ID"];
            TaskStats task(taskId);
            if (taskObj["profiles"].is<JsonArray>()) {
                JsonArray profilesArr = taskObj["profiles"];
                for (JsonObject profObj : profilesArr) {
                    long profId = profObj["ID"];
                    ProfileStats prof(taskId, profId);
                    prof.runs = profObj["stats"]["runs"] | 0;
                    prof.products = profObj["stats"]["products"] | 0;
                    prof.paper_um = static_cast<uint64_t>((profObj["stats"]["paper_mm"] | 0.0f) * 1000.0f);
                    prof.sheets = profObj["stats"]["sheets"] | 0;
                    prof.rejects = profObj["stats"]["rejects"] | 0;
                    if (profObj["stats"]["steps"].is<JsonObject>()) {
                        JsonObject stepsObj = profObj["stats"]["steps"];
                        for (JsonPair kv : stepsObj) {
                            prof.steps[String(kv.key().c_str())] = kv.value().as<uint64_t>();
                        }
                    }
                    task.profiles.push_back(prof);
                }
            }
            tasks.push_back(task);
        }
    }

    Log::D("Stats: loaded from file");
    return true;
}

// Сохранение в JSON
bool Stats::saveToFile() {
    if (!enabled) return true;

    JsonDocument doc;

    // Device
    JsonObject devObj = doc["device"].to<JsonObject>();
    devObj["power_on"] = device.power_on;
    JsonObject devStepsObj = devObj["steps"].to<JsonObject>();
    for (auto& kv : device.steps) {
        devStepsObj[kv.first.c_str()] = kv.second;
    }

    // Operation
    JsonObject opObj = doc["operation"].to<JsonObject>();
    JsonArray tasksArr = opObj["tasks"].to<JsonArray>();
    for (auto& task : tasks) {
        JsonObject taskObj = tasksArr.add<JsonObject>();
        taskObj["ID"] = task.id;
        JsonArray profilesArr = taskObj["profiles"].to<JsonArray>();
        for (auto& prof : task.profiles) {
            JsonObject profObj = profilesArr.add<JsonObject>();
            profObj["ID"] = prof.profileId;
            JsonObject statsObj = profObj["stats"].to<JsonObject>();
            statsObj["runs"] = prof.runs;
            statsObj["products"] = prof.products;
            statsObj["paper_mm"] = prof.paper_um / 1000.0f;
            statsObj["sheets"] = prof.sheets;
            statsObj["rejects"] = prof.rejects;
            JsonObject stepsObj = statsObj["steps"].to<JsonObject>();
            for (auto& kv : prof.steps) {
                stepsObj[kv.first.c_str()] = kv.second;
            }
        }
    }

    File file = LittleFS.open(STATS_PATH, "w");
    if (!file) {
        Log::D("Stats: cannot open file for write");
        return false;
    }

    serializeJson(doc, file);
    file.close();

    Log::D("Stats: saved to file");
    return true;
}

// Поиск или создание TaskStats
Stats::TaskStats* Stats::findOrCreateTask(long taskId) {
    for (auto& task : tasks) {
        if (task.id == taskId) {
            return &task;
        }
    }
    tasks.emplace_back(taskId);
    return &tasks.back();
}

// Поиск ProfileStats в TaskStats
Stats::ProfileStats* Stats::findOrCreateProfile(TaskStats* task, long profileId) {
    for (auto& prof : task->profiles) {
        if (prof.profileId == profileId) {
            return &prof;
        }
    }
    task->profiles.emplace_back(task->id, profileId);
    return &task->profiles.back();
}

// Методы учёта
void Stats::jobStart(long taskId, long profileId, float ratioStepsPerMm) {
    if (!enabled) return;
    currentTaskId = taskId;
    currentProfileId = profileId;
    currentRatio = ratioStepsPerMm;
    jobActive = true;

    TaskStats* task = findOrCreateTask(taskId);
    ProfileStats* prof = findOrCreateProfile(task, profileId);
    prof->runs++;

    Log::D("Stats: jobStart task=%ld profile=%ld", taskId, profileId);
}

void Stats::jobFinish() {
    if (!enabled || !jobActive) return;
    jobActive = false;
    saveToFile();
    Log::D("Stats: jobFinish");
}

void Stats::onMotorSteps(const String& motorName, long steps) {
    if (!enabled) return;

    uint64_t absSteps = steps < 0 ? static_cast<uint64_t>(-steps) : static_cast<uint64_t>(steps);

    // Device
    device.steps[motorName] += absSteps;

    // Operation, если job активен
    if (jobActive) {
        TaskStats* task = findOrCreateTask(currentTaskId);
        ProfileStats* prof = findOrCreateProfile(task, currentProfileId);
        prof->steps[motorName] += absSteps;

        // Метраж бумаги только для PAPER мотора
        if (motorName == "PAPER" && currentRatio > 0.0f) {
            prof->paper_um += static_cast<uint64_t>((static_cast<float>(absSteps) / currentRatio) * 1000.0f);
        }
    }
}

void Stats::onSheet() {
    if (!enabled || !jobActive) return;
    TaskStats* task = findOrCreateTask(currentTaskId);
    ProfileStats* prof = findOrCreateProfile(task, currentProfileId);
    prof->sheets++;
}

void Stats::onProduct() {
    if (!enabled || !jobActive) return;
    TaskStats* task = findOrCreateTask(currentTaskId);
    ProfileStats* prof = findOrCreateProfile(task, currentProfileId);
    prof->products++;
}

void Stats::onReject() {
    if (!enabled || !jobActive) return;
    TaskStats* task = findOrCreateTask(currentTaskId);
    ProfileStats* prof = findOrCreateProfile(task, currentProfileId);
    prof->rejects++;
}

void Stats::save() {
    if (!enabled) return;
    saveToFile();
}

std::vector<Stats::MotorStat> Stats::getDeviceMotors() const {
    return mapToMotors(device.steps);
}

std::vector<Stats::TaskRunStat> Stats::getTaskRuns() const {
    std::vector<TaskRunStat> result;
    result.reserve(tasks.size());
    for (const auto& task : tasks) {
        uint64_t runs = 0;
        for (const auto& prof : task.profiles) {
            runs += prof.runs;
        }
        TaskRunStat item;
        item.id = task.id;
        item.runs = runs;
        result.push_back(item);
    }
    std::sort(result.begin(), result.end(),
        [](const TaskRunStat& a, const TaskRunStat& b) { return a.runs > b.runs; });
    return result;
}

std::vector<Stats::ProfileRunStat> Stats::getProfileRuns() const {
    std::map<long, uint64_t> runsByProfile;
    for (const auto& task : tasks) {
        for (const auto& prof : task.profiles) {
            runsByProfile[prof.profileId] += prof.runs;
        }
    }

    std::vector<ProfileRunStat> result;
    result.reserve(runsByProfile.size());
    for (const auto& kv : runsByProfile) {
        ProfileRunStat item;
        item.id = kv.first;
        item.runs = kv.second;
        result.push_back(item);
    }
    std::sort(result.begin(), result.end(),
        [](const ProfileRunStat& a, const ProfileRunStat& b) { return a.runs > b.runs; });
    return result;
}

std::vector<Stats::ProfileRunStat> Stats::getTaskProfileRuns(long taskId) const {
    std::vector<ProfileRunStat> result;
    for (const auto& task : tasks) {
        if (task.id != taskId) continue;
        result.reserve(task.profiles.size());
        for (const auto& prof : task.profiles) {
            ProfileRunStat item;
            item.id = prof.profileId;
            item.runs = prof.runs;
            result.push_back(item);
        }
        break;
    }
    std::sort(result.begin(), result.end(),
        [](const ProfileRunStat& a, const ProfileRunStat& b) { return a.runs > b.runs; });
    return result;
}

bool Stats::getTaskAggregate(long taskId, AggregateStat& out) const {
    resetAggregate(out);
    for (const auto& task : tasks) {
        if (task.id != taskId) continue;
        std::map<String, uint64_t> steps;
        out.profiles = static_cast<uint32_t>(task.profiles.size());
        for (const auto& prof : task.profiles) {
            out.runs += prof.runs;
            out.products += prof.products;
            out.paper_um += prof.paper_um;
            out.sheets += prof.sheets;
            out.rejects += prof.rejects;
            addSteps(steps, prof.steps);
        }
        out.motors = mapToMotors(steps);
        return true;
    }
    return false;
}

bool Stats::getTaskProfileAggregate(long taskId, long profileId, AggregateStat& out) const {
    resetAggregate(out);
    std::map<String, uint64_t> steps;
    bool found = false;
    for (const auto& task : tasks) {
        if (task.id != taskId) continue;
        for (const auto& prof : task.profiles) {
            if (prof.profileId != profileId) continue;
            found = true;
            out.runs += prof.runs;
            out.products += prof.products;
            out.paper_um += prof.paper_um;
            out.sheets += prof.sheets;
            out.rejects += prof.rejects;
            addSteps(steps, prof.steps);
        }
        break;
    }
    out.motors = mapToMotors(steps);
    return found;
}

bool Stats::getProfileAggregate(long profileId, AggregateStat& out) const {
    resetAggregate(out);
    std::map<String, uint64_t> steps;
    bool found = false;
    for (const auto& task : tasks) {
        for (const auto& prof : task.profiles) {
            if (prof.profileId != profileId) continue;
            found = true;
            out.runs += prof.runs;
            out.products += prof.products;
            out.paper_um += prof.paper_um;
            out.sheets += prof.sheets;
            out.rejects += prof.rejects;
            addSteps(steps, prof.steps);
        }
    }
    out.motors = mapToMotors(steps);
    return found;
}

// Отладка
void Stats::printState() {
    if (!enabled) {
        Log::D("Stats: disabled");
        return;
    }

    Log::D("Stats RAM state:");
    Log::D("Device:");
    Log::D("  power_on: %u", device.power_on);
    Log::D("  steps:");
    for (auto& kv : device.steps) {
        Log::D("    %s: %llu", kv.first.c_str(), kv.second);
    }

    Log::D("Operation:");
    for (auto& task : tasks) {
        Log::D("  Task ID: %ld", task.id);
        for (auto& prof : task.profiles) {
            Log::D("    Profile ID: %ld", prof.profileId);
            Log::D("      runs: %u, products: %llu, paper_mm: %.3f, sheets: %llu, rejects: %llu",
                    prof.runs, prof.products, prof.paper_um / 1000.0f, prof.sheets, prof.rejects);
            Log::D("      steps:");
            for (auto& kv : prof.steps) {
                Log::D("        %s: %llu", kv.first.c_str(), kv.second);
            }
        }
    }
}

void Stats::printSnapshot() {
    if (!enabled) {
        Log::D("Stats: disabled");
        return;
    }

    if (!LittleFS.exists(STATS_PATH)) {
        Log::D("Stats file does not exist");
        return;
    }

    File file = LittleFS.open(STATS_PATH, "r");
    if (!file) {
        Log::D("Cannot open stats file");
        return;
    }

    size_t size = file.size();
    Log::D("Stats file size: %d bytes", size);

    String content;
    while (file.available()) {
        content += (char)file.read();
    }
    file.close();

    Log::D("Stats file content:");
    Log::D("%s", content.c_str());
}

void Stats::printFsUsage() {
    size_t total = LittleFS.totalBytes();
    size_t used = LittleFS.usedBytes();
    Log::D("LittleFS: total %d bytes, used %d bytes, free %d bytes", total, used, total - used);
}

bool Stats::clearAll() {
    tasks.clear();
    device.steps.clear();
    device.power_on = 0;
    currentTaskId = -1;
    currentProfileId = -1;
    currentRatio = 0.0f;
    jobActive = false;
    return FileSystem::deleteFile(STATS_PATH);
}
