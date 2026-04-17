#pragma once

#include <Arduino.h>
#include <vector>

#include "Catalog.h"

class DeviceError {
public:
    using ErrorCode = Catalog::ErrorCode;
    using ErrorType = Catalog::ErrorType;
    using StateType = Catalog::State;

    enum class Kind : uint8_t {
        Warning = 0,
        Error = 1,
        Fatal = 2
    };

    struct Entry {
        ErrorType error;
        Kind kind = Kind::Error;
        uint32_t timestampMs = 0;
    };

    static DeviceError& getInstance() {
        static DeviceError instance;
        return instance;
    }

    void clear() {
        list.clear();
        cursor = 0;
        collecting = false;
    }

    void clearErrors() {
        for (size_t i = list.size(); i > 0; --i) {
            const Kind kind = list[i - 1].kind;
            if (kind == Kind::Error || kind == Kind::Fatal) {
                list.erase(list.begin() + static_cast<long>(i - 1));
            }
        }
        normalizeCursor();
    }

    void clearWarnings() {
        for (size_t i = list.size(); i > 0; --i) {
            if (list[i - 1].kind == Kind::Warning) {
                list.erase(list.begin() + static_cast<long>(i - 1));
            }
        }
        normalizeCursor();
    }

    void setCollecting(bool enable) { collecting = enable; }
    bool isCollecting() const { return collecting; }

    // Backward-compatible alias.
    StateType add(ErrorCode code, const String& description = "", const String& value = "") {
        return addFatal(code, description, value);
    }

    StateType addFatal(ErrorCode code,
                       const String& description = "",
                       const String& value = "") {
        Entry entry;
        entry.error.code = code;
        entry.error.description = description;
        entry.error.value = value;
        entry.kind = Kind::Fatal;
        entry.timestampMs = millis();
        list.push_back(entry);

        if (list.size() == 1) cursor = 0;
        return StateType::ERROR;
    }

    StateType addError(ErrorCode code,
                       const String& description = "",
                       const String& value = "",
                       bool stopMotion = false) {
        if (stopMotion) {
            return addFatal(code, description, value);
        }

        Entry entry;
        entry.error.code = code;
        entry.error.description = description;
        entry.error.value = value;
        entry.kind = Kind::Error;
        entry.timestampMs = millis();
        list.push_back(entry);

        if (list.size() == 1) cursor = 0;
        return collecting ? StateType::DONE : StateType::ERROR;
    }

    // Warning is informational only and does not affect state transitions.
    StateType addWarning(ErrorCode code, const String& description = "", const String& value = "") {
        Entry entry;
        entry.error.code = code;
        entry.error.description = description;
        entry.error.value = value;
        entry.kind = Kind::Warning;
        entry.timestampMs = millis();
        list.push_back(entry);

        if (list.size() == 1) cursor = 0;
        return StateType::DONE;
    }

    // Legacy semantics: hasAny() checks only errors (warning must not trigger ERROR state).
    bool hasAny() const { return hasErrors(); }

    bool hasErrors() const {
        for (const Entry& item : list) {
            if (item.kind == Kind::Error || item.kind == Kind::Fatal) return true;
        }
        return false;
    }

    bool hasWarnings() const {
        for (const Entry& item : list) {
            if (item.kind == Kind::Warning) return true;
        }
        return false;
    }

    bool hasMessages() const { return !list.empty(); }
    bool hasBlockingError() const { return hasErrors(); }

    bool hasStopMotionError() const {
        for (const Entry& item : list) {
            if (item.kind == Kind::Fatal) return true;
        }
        return false;
    }

    bool hasFatal() const { return hasStopMotionError(); }

    size_t count() const { return list.size(); }

    bool canNext() const { return cursor + 1 < list.size(); }
    bool canPrev() const { return cursor > 0; }

    bool next() {
        if (!canNext()) return false;
        ++cursor;
        return true;
    }

    bool prev() {
        if (!canPrev()) return false;
        --cursor;
        return true;
    }

    void resetCursor() { cursor = 0; }
    size_t currentIndex() const { return cursor; }

    const ErrorType& current() const { return list[cursor].error; }
    const ErrorType& currentOrNoError() const {
        static ErrorType noError{ErrorCode::NO_ERROR, "", ""};
        if (list.empty()) return noError;
        return list[cursor].error;
    }

    const Entry& currentEntry() const { return list[cursor]; }

    bool currentIsFatal() const {
        if (list.empty()) return false;
        return list[cursor].kind == Kind::Fatal;
    }

private:
    std::vector<Entry> list;
    size_t cursor = 0;
    bool collecting = false;

    void normalizeCursor() {
        if (list.empty()) {
            cursor = 0;
            return;
        }
        if (cursor >= list.size()) cursor = list.size() - 1;
    }

    DeviceError() = default;
};
