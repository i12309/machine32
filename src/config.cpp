#include "config.h"
#include "Machine/MachineSpec.h"

namespace ConfigDefaults {

namespace detail {

void fillSettings(JsonObject settings, bool accessPoint) {
    settings["AUTO_UPDATE"] = 0;
    settings["UPDATE"] = 1;
    settings["SERVER"] = "45.9.43.227";
    settings["VERSION"] = "firmware.txt";
    settings["FIRMWARE"] = "firmware.bin";
    settings["HASH"] = "firmware.md5";

    settings["TFT_UPDATE"] = 1;
    settings["TFT_VERSION"] = "ui.txt";
    settings["TFT_FIRMWARE"] = "ui.tft";

    settings["CONNECT_WIFI"] = 1;
    settings["ACCESS_POINT"] = accessPoint ? 1 : 0;
    settings["HTTP_SERVER"] = 1;
    settings["WEB"] = 0;

    settings["LOG_BUFFER"] = 0;
    settings["CHECK_SYSTEM"] = 1;
    settings["ALLOW_MISSING_HARDWARE"] = 0;
    settings["licence_off"] = 1;
    settings["MAX_PROFILES"] = 50;
    settings["MAX_TASKS"] = 50;
    settings["log"] = 0;
    settings["metrics"] = 1;
    // Отдельный таймаут для McpTrigger: позволяет переживать короткую занятость I2C без ложного отказа arm/read snapshot.
    settings["MCP_TRIGGER_I2C_WAIT_MS"] = 150;
}

bool fillDevice(JsonObject device, Catalog::MachineType type) {
    MachineSpec spec = MachineSpec::get(type);
    if (spec.type() == Catalog::MachineType::UNKNOWN) {
        Log::E("[ConfigDefaults] MachineSpec is not implemented for type '%s'", Catalog::machineName(type).c_str());
        return false;
    }

    spec.fillDeviceDefaults(device);
    return true;
}

void fillTuning(JsonObject tuning) {
    tuning["EDGE_DISTANCE_mm"] = 17;
    tuning["SENSOR_DISTANCE_mm"] = 17;
    tuning["DELTA_mm"] = 0;
    tuning["MARK_LENGHT_mm"] = 3;
    tuning["MARK_CENTER_DISTANCE_mm"] = 9.5;
    tuning["OFFSET_FIRSTCUT_MM"] = 8.0;
    tuning["OFFSET_TOL_MM"] = 0.15;
    tuning["OFFSET_MAX_ITER"] = 2;
    tuning["OVER_mm"] = 2;
    tuning["DISTANCE_BETWEEN_MARKS_mm"] = 40;
    tuning["CUT_count"] = 12;
    tuning["PROFILE_WIDTH_step"] = 2000;
    tuning["PROFILE_COUNT_CUT"] = 5;
}

void fillTestProfiles(JsonArray profiles) {
    JsonObject profile = profiles.add<JsonObject>();
    profile["ID"] = 0;
    profile["NAME"] = "Профиль";
    profile["RATIO_mm"] = 50.6;
    profile["DESC"] = "Описание профиля";
    profile["LENGHT_mm"] = 450;
}

void fillTestTasks(JsonArray tasks) {
    JsonObject task = tasks.add<JsonObject>();
    task["ID"] = 0;
    task["NAME"] = "Задание";
    task["PROFILE_ID"] = 0;
    task["OVER_mm"] = 2;
    task["PRODUCT_mm"] = 49.5;
    task["FIRST_CUT_mm"] = 8;
    task["MARK_mm"] = 3;
    task["LASTCUT_mm"] = 0;
    task["MARK"] = 1;
}

} // namespace detail

bool build(JsonDocument& doc, const Options& options) {
    doc.clear();

    JsonObject root = doc.to<JsonObject>();
    root["config_version"] = options.configVersion;
    root["machine"] = Catalog::machineName(options.machine);
    root["name"] = options.name;
    root["group"] = options.group;

    JsonObject settings = root["settings"].to<JsonObject>();
    detail::fillSettings(settings, options.accessPoint);

    JsonObject device = root["device"].to<JsonObject>();
    if (!detail::fillDevice(device, options.machine)) {
        return false;
    }

    JsonObject tuning = root["tuning"].to<JsonObject>();
    detail::fillTuning(tuning);

    return true;
}

bool buildData(JsonDocument& doc, bool withTestData) {
    doc.clear();

    JsonObject root = doc.to<JsonObject>();

    JsonArray profiles = root["profiles"].to<JsonArray>();
    JsonArray tasks = root["tasks"].to<JsonArray>();

    if (withTestData) {
        detail::fillTestProfiles(profiles);
        detail::fillTestTasks(tasks);
    }

    return true;
}

} // namespace ConfigDefaults
