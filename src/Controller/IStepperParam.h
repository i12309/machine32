#pragma once

#include <ArduinoJson.h>

#include <cctype>
#include <cstring>
#include <map>
#include <string>

#include "Catalog.h"

class IStepperParam {
public:
    struct SpeedConfig {
        int32_t speed = 0;
        int32_t acceleration = 0;
    };

    void load(JsonObjectConst configObj) {
        clear();

        const int32_t normalSpeed = configObj["MaxSpeed"] | 0;
        const int32_t normalAcceleration = configObj["Acceleration"] | 0;
        _params["MicroStep"] = configObj["MicroStep"] | 0;
        setSpeedProfile("Normal", normalSpeed, normalAcceleration);

        JsonArrayConst speedArray = configObj["Speed"].as<JsonArrayConst>();
        if (!speedArray.isNull()) {
            for (JsonObjectConst speedObj : speedArray) {
                std::string mode = speedObj["Mode"] | "";
                if (mode.empty()) continue;

                int32_t speed = speedObj["Speed"] | -1;
                if (speed < 0) continue;

                int32_t acceleration = speedObj["Acceleration"] | -1;
                if (acceleration < 0) acceleration = normalAcceleration;

                setSpeedProfile(mode, speed, acceleration);
            }
        }

        JsonArrayConst checkArray = configObj["Check"].as<JsonArrayConst>();
        if (!checkArray.isNull()) {
            for (JsonObjectConst checkObj : checkArray) {
                std::string mode = checkObj["Mode"] | "";
                if (mode.empty()) continue;
                if (!checkObj["step"].is<int32_t>()) continue;

                const int32_t step = checkObj["step"].as<int32_t>();
                const int32_t ms = checkObj["ms"] | -1;
                setCheckProfile(mode, step, ms);
            }
        }

        for (JsonPairConst kvp : configObj) {
            std::string key = kvp.key().c_str();
            if (key == "MicroStep" || key == "MaxSpeed" || key == "Acceleration"
                || key == "I2C" || key == "Speed" || key == "Check" || key == "pin"
                || key == "step_pin" || key == "dir_pin" || key == "ena_pin") {
                continue;
            }

            if (key != "MaxSpeed" && endsWith(key, "Speed") && kvp.value().is<int32_t>()) {
                const std::string mode = key.substr(0, key.size() - std::strlen("Speed"));
                const int32_t speed = kvp.value().as<int32_t>();
                int32_t acceleration = normalAcceleration;
                const std::string accelerationKey = mode + "Acceleration";
                if (configObj[accelerationKey.c_str()].is<int32_t>()) {
                    acceleration = configObj[accelerationKey.c_str()].as<int32_t>();
                }
                setSpeedProfile(mode, speed, acceleration);
                continue;
            }

            if (key != "Acceleration" && endsWith(key, "Acceleration")) {
                continue;
            }

            if (key.rfind("Check", 0) == 0) {
                if (endsWith(key, "_ms")) continue;
                if (!kvp.value().is<int32_t>()) continue;

                const std::string mode = key.substr(std::strlen("Check"));
                if (!mode.empty()) {
                    const int32_t step = kvp.value().as<int32_t>();
                    const std::string msKey = key + "_ms";
                    const int32_t ms = configObj[msKey.c_str()] | -1;
                    setCheckProfile(mode, step, ms);
                }
                continue;
            }

            if (kvp.value().is<int32_t>()) {
                _params[key] = kvp.value().as<int32_t>();
            }
        }

        ensureNormalSpeed();
    }

    void clear() {
        _params.clear();
        _speedProfiles.clear();
        _checkProfiles.clear();
    }

    static std::string speedModeName(Catalog::SPEED speed) {
        switch (speed) {
            case Catalog::SPEED::Slow: return "Slow";
            case Catalog::SPEED::Fast: return "Fast";
            case Catalog::SPEED::Force: return "Force";
            default: return "Normal";
        }
    }

    SpeedConfig resolveSpeed(Catalog::SPEED speed) const {
        return resolveSpeed(speedModeName(speed));
    }

    SpeedConfig resolveSpeed(const std::string& mode) const {
        SpeedConfig normal;
        normal.speed = 0;
        normal.acceleration = 0;

        auto normalIt = _speedProfiles.find(normalizeKey("Normal"));
        if (normalIt != _speedProfiles.end()) {
            normal = normalIt->second;
        }

        auto it = _speedProfiles.find(normalizeKey(mode));
        if (it == _speedProfiles.end()) return normal;

        SpeedConfig result = it->second;
        if (result.speed <= 0) result.speed = normal.speed;
        if (result.acceleration <= 0) result.acceleration = normal.acceleration;
        return result;
    }

    bool hasSpeed(const std::string& mode) const {
        return _speedProfiles.find(normalizeKey(mode)) != _speedProfiles.end();
    }

    int32_t getSpeed(const std::string& mode = "Normal") const {
        return resolveSpeed(mode).speed;
    }

    int32_t getAcceleration(const std::string& mode) const {
        return resolveSpeed(mode).acceleration;
    }

    bool hasCheck(const std::string& mode) const {
        return _checkProfiles.find(normalizeKey(mode)) != _checkProfiles.end();
    }

    int32_t getCheck(const std::string& mode, int32_t defaultValue = 0) const {
        auto it = _checkProfiles.find(normalizeKey(mode));
        if (it != _checkProfiles.end()) return it->second.step;
        return defaultValue;
    }

    int32_t getCheckMs(const std::string& mode, int32_t defaultValue = -1) const {
        auto it = _checkProfiles.find(normalizeKey(mode));
        if (it != _checkProfiles.end()) return it->second.ms;
        return defaultValue;
    }

    int32_t getValue(const std::string& param, int32_t defaultValue = 0) const {
        auto it = _params.find(param);
        if (it != _params.end()) return it->second;
        return defaultValue;
    }

    int32_t getValue(const std::string& mode, const std::string& param, int32_t defaultValue = 0) const {
        if (mode.empty()) return getValue(param, defaultValue);

        const std::string key = normalizeKey(param);
        if (key == "speed") return resolveSpeed(mode).speed;
        if (key == "acceleration") return resolveSpeed(mode).acceleration;
        return defaultValue;
    }

private:
    struct CheckConfig {
        int32_t step = 0;
        int32_t ms = -1;
    };

    std::map<std::string, int32_t> _params;
    std::map<std::string, SpeedConfig> _speedProfiles;
    std::map<std::string, CheckConfig> _checkProfiles;

    static std::string normalizeKey(const std::string& value) {
        std::string result;
        result.reserve(value.size());
        for (char c : value) {
            if (std::isspace(static_cast<unsigned char>(c))) continue;
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        return result;
    }

    static bool endsWith(const std::string& value, const char* suffix) {
        const size_t suffixLen = std::strlen(suffix);
        if (value.size() < suffixLen) return false;
        return value.compare(value.size() - suffixLen, suffixLen, suffix) == 0;
    }

    void setSpeedProfile(const std::string& mode, int32_t speed, int32_t acceleration) {
        if (mode.empty()) return;
        SpeedConfig profile;
        profile.speed = speed;
        profile.acceleration = acceleration;
        _speedProfiles[normalizeKey(mode)] = profile;
    }

    void setCheckProfile(const std::string& mode, int32_t step, int32_t ms = -1) {
        if (mode.empty()) return;
        CheckConfig profile;
        profile.step = step;
        profile.ms = ms;
        _checkProfiles[normalizeKey(mode)] = profile;
    }

    void ensureNormalSpeed() {
        const std::string normalKey = normalizeKey("Normal");
        auto normalIt = _speedProfiles.find(normalKey);
        if (normalIt == _speedProfiles.end()) {
            if (!_speedProfiles.empty()) {
                normalIt = _speedProfiles.emplace(normalKey, _speedProfiles.begin()->second).first;
            } else {
                SpeedConfig profile;
                normalIt = _speedProfiles.emplace(normalKey, profile).first;
            }
        }

        if (normalIt->second.speed < 0) normalIt->second.speed = 0;
        if (normalIt->second.acceleration < 0) normalIt->second.acceleration = 0;

        const SpeedConfig normal = normalIt->second;
        for (auto& kv : _speedProfiles) {
            if (kv.second.speed <= 0) kv.second.speed = normal.speed;
            if (kv.second.acceleration <= 0) kv.second.acceleration = normal.acceleration;
        }
    }
};
