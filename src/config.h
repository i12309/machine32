#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "Catalog.h"
#include "version.h"

#define FORMAT_LITTLEFS_IF_FAILED true

const String CONFIG_PATH = "/config.json";
const String DATA_PATH = "/data.json";

namespace ConfigDefaults {

struct Options {
    Catalog::MachineType machine = Catalog::MachineType::A;
    String group = "DEV";
    String name = "ESP32";
    bool accessPoint = false;
    bool withTestData = false;
    int configVersion = 0;
};

namespace detail {

void fillSettings(JsonObject settings, bool accessPoint);
bool fillDevice(JsonObject device, Catalog::MachineType type);
void fillTuning(JsonObject tuning);
void fillTestProfiles(JsonArray profiles);
void fillTestTasks(JsonArray tasks);

} // namespace detail

bool build(JsonDocument& doc, const Options& options);
bool buildData(JsonDocument& doc, bool withTestData = false);

} // namespace ConfigDefaults
