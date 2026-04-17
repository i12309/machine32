#pragma once

#include <Arduino.h>
#include "Service/Log.h"

// Centralized catalog of machine-related enumerations and helpers.
class Catalog {
public:
    enum class MachineType {
        UNKNOWN,
        A, // Нарезчик
        B, // Беговщик
        C, // Беговка с нарезкой
        D, // Универсальный 
        E, // 
        F, // 
    };

    enum class Mode {
        NORMAL,
        SERVICE
    };

    enum class FormMode {
        VIEW,
        EDIT,
        CREATE
    };

    enum class PageMode {
        pMain,
        pService,
        pTaskRun,
        pTask,
        pCalibration,
        pSlice
    };

    enum class State {
        NULL_STATE,
        IDLE,
        FINISH,
        ERROR,
        FRAME,
        DONE, // Специальное состояние для завершения и возврата в вызывающее состояние
        ACTION,

        BOOT,
        PRESSURE,// Сервисное для измерений давления
        CHECK_PAPER,
        CHECK_GUILLOTINE,
        CHECK_TABLE,

        TABLE_UP,
        TABLE_DOWN,
        DETECT_PAPER,
        DETECT_MARK,
        GUILLOTINE_FORWARD,
        GUILLOTINE_BACKWARD,
        PAPER_MOVE,
        PAPER_PULL,
        //PAPER_OUT,

        // Станок
        PROCESS,
        CHECK,
        SERVICE,
        CALIBRATION,
        PROFILING,
        SLICE,

        TEST,
        T100,
        T100B,
        TDL,
        TCUT,
    };

    static String getStateName(State type) {
        switch (type) {
            case State::NULL_STATE:        return "NULLSTATE";
            case State::IDLE:              return "IDLE";
            case State::FINISH:            return "FINISH";
            case State::ERROR:             return "ERROR";
            case State::FRAME:             return "FRAME";
            case State::DONE:                return "DONE";
            case State::ACTION:              return "ACTION";

            case State::TABLE_UP:          return "TABLE_UP";
            case State::TABLE_DOWN:        return "TABLE_DOWN";
            case State::DETECT_PAPER:      return "DETECT_PAPER";
            case State::DETECT_MARK:       return "DETECT_MARK";
            case State::GUILLOTINE_FORWARD:return "GUILLOTINE_FORWARD";
            case State::GUILLOTINE_BACKWARD:return "GUILLOTINE_BACKWARD";
            case State::PAPER_MOVE:        return "PAPER_MOVE";
            case State::PAPER_PULL:        return "PAPER_PULL";
            //case State::PAPER_OUT:          return "PAPER_OUT";   

            case State::BOOT:              return "BOOT";
            case State::PRESSURE:             return "PRESSURE";
            case State::CHECK_PAPER:         return "CHECK_PAPER";
            case State::CHECK_GUILLOTINE:    return "CHECK_GUILLOTINE";
            case State::CHECK_TABLE:         return "CHECK_TABLE";

            // Станок
            case State::PROCESS:           return "PROCESS";
            case State::CHECK:             return "CHECK";
            case State::SERVICE:           return "SERVICE";
            case State::CALIBRATION:       return "CALIBRATION";
            case State::PROFILING:         return "PROFILING";
            case State::SLICE:             return "SLICE";

             case State::TEST:              return "TEST";

            default:                      return "UNKNOWN_STATE";
        }
    }

    struct Color {
        // Named colors from doc/color.md
        static constexpr uint32_t green       = 901;      // Зеленый
        static constexpr uint32_t blue        = 537;      // Синий
        static constexpr uint32_t orange      = 60548;    // Оранжевый
        static constexpr uint32_t red         = 53734;    // Красный
        static constexpr uint32_t yellow      = 62944;    // Желтый
        static constexpr uint32_t cyan        = 1151;     // Голубой
        
        // Grayscale variants (от темного к светлому)
        static constexpr uint32_t black       = 16938;    // Самый темный
        static constexpr uint32_t darkGrey    = 23275;    // Темно-серый
        static constexpr uint32_t grey        = 31695;    // Серый
        static constexpr uint32_t mediumGrey  = 44373;    // Средний серый
        static constexpr uint32_t lightGrey   = 52857;    // Светлый серый
        static constexpr uint32_t lighter     = 59196;    // Еще светлее
        static constexpr uint32_t lightest    = 63390;    // Очень светлый
        static constexpr uint32_t white       = 65535;    // Белый
    };

    struct UI {
        // Picture indices for switch states
        static constexpr uint32_t sw_pic1 = 29;  // Off state picture
        static constexpr uint32_t sw_pic2 = 30;  // On state picture
    };

    enum class ErrorCode : uint8_t {
        NO_ERROR = 0,
        UNKNOWN_ERROR = 1,
        SENSOR_FAULT = 2,
        MOTOR_STALL = 3,
        CONFIG_ERROR = 4,
        BRAKING_PROCESS = 5,

        CHECK_FAILED = 10,
        CALIBRATION_FAILED = 20,
        PROFILING_ERROR = 21,

        GUILLOTINE = 30,
        GUILLOTINE_NOT_OUT = 31,
        GUILLOTINE_NOT_IN = 32,
        GUILLOTINE_PIN_NOT_CONNECT = 33,
        GUILLOTINE_SENSOR_PIN_NOT_CONNECT = 34, 

        TABLE = 40,
        TABLE_NOT_OUT = 41,
        TABLE_NOT_IN = 42,
        TABLE_PIN_NOT_CONNECT = 43,
        TABLE_SENSOR_PIN_NOT_CONNECT = 44,
        TABLE_NOT_UP = 45,   // Таймаут при движении вверх: стол не поднялся
        TABLE_NOT_DOWN = 46, // Таймаут при движении вниз: стол не опустился

        CLUTCH_PIN_NOT_CONNECT = 50,

        PAPER = 100,
        PAPER_JAM = 101,
        PAPER_FIND_IN_MARK = 102,
        PAPER_PIN_NOT_CONNECT = 103,
        PAPER_SENSOR_PIN_NOT_CONNECT = 104,
        PAPER_NOT_FOUND = 105,
        PAPER_FIND_OUT_MARK = 106,
        PAPER_SEARCH = 107
    };

    struct ErrorType {
        ErrorCode code;
        String description;
        String value;
    };

    static String machineName(MachineType machine) {
        switch(machine) {
            case MachineType::A: return "A";
            case MachineType::B: return "B";
            case MachineType::C: return "C";
            case MachineType::D: return "D";
            case MachineType::E: return "E";
            case MachineType::F: return "F";
            case MachineType::UNKNOWN:
            default: return "UNKNOWN";
        }
    }

    static MachineType getMachine(String name) {
        MachineType machine = MachineType::UNKNOWN;
        if (name == "A") machine = MachineType::A;
        if (name == "B") machine = MachineType::B;
        if (name == "C") machine = MachineType::C;
        if (name == "D") machine = MachineType::D;
        if (name == "E") machine = MachineType::E;
        if (name == "F") machine = MachineType::F;
        return machine;
    }

    static const MachineType* machineTypes(size_t& count) {
        static const MachineType types[] = {
            MachineType::A,
            MachineType::B,
            MachineType::C,
            MachineType::D,
            MachineType::E,
            MachineType::F
        };
        count = sizeof(types) / sizeof(types[0]);
        return types;
    }

    static String modeName(Mode mode) {
        switch(mode) {
            case Mode::NORMAL: return "NORMAL";
            case Mode::SERVICE: return "SERVICE";
            default: return "NULL";
        }
    }

    static String errorName(ErrorCode code) {
        switch(code){
            case ErrorCode::NO_ERROR: return "  Нет ошибки";
            case ErrorCode::UNKNOWN_ERROR: return "  Неизвестная ошибка";
            case ErrorCode::SENSOR_FAULT: return "  Неисправность датчика";
            case ErrorCode::MOTOR_STALL: return "  Неисправность двигателя";
            case ErrorCode::CONFIG_ERROR: return "  Не верные настройки устройства";
            case ErrorCode::BRAKING_PROCESS: return "  Процесс прерван";

            case ErrorCode::CHECK_FAILED: return "  Ошибка проверки системы ";
            case ErrorCode::CALIBRATION_FAILED: return "  Ошибка калибровки    ";
            case ErrorCode::PROFILING_ERROR: return "  Ошибка профилирования ";

            case ErrorCode::GUILLOTINE: return "  Ошибка гильотины или ее датчика";
            case ErrorCode::GUILLOTINE_NOT_OUT: return "  Гильотина не вышла из датчика";
            case ErrorCode::GUILLOTINE_NOT_IN: return "  Гильотина не вернулась в датчик";
            case ErrorCode::GUILLOTINE_PIN_NOT_CONNECT: return "  Мотор гильотины не подключен";
            case ErrorCode::GUILLOTINE_SENSOR_PIN_NOT_CONNECT: return "  Датчик гильотины не подключен";

            case ErrorCode::TABLE: return "  Ошибка стола или ее датчика";
            case ErrorCode::TABLE_NOT_OUT: return "  Стол не вышл из датчика";
            case ErrorCode::TABLE_NOT_IN: return "  Стол не вернулся в датчик";
            case ErrorCode::TABLE_PIN_NOT_CONNECT: return "  Мотор стола не подключен";
            case ErrorCode::TABLE_SENSOR_PIN_NOT_CONNECT: return "  Датчик стола не подключен";
            case ErrorCode::TABLE_NOT_UP: return "  Стол не поднялся (таймаут)";
            case ErrorCode::TABLE_NOT_DOWN: return "  Стол не опустился (таймаут)";

            case ErrorCode::CLUTCH_PIN_NOT_CONNECT: return "  Муфта не подключен пин";

            case ErrorCode::PAPER: return "  Ошибка главного мотора";
            case ErrorCode::PAPER_NOT_FOUND: return "  Бумага не найдена";
            case ErrorCode::PAPER_SEARCH: return "  Превышен интервал ожидания";
            case ErrorCode::PAPER_JAM: return "  Зажевание бумаги";
            case ErrorCode::PAPER_FIND_IN_MARK: return "  Не найдена метка";
            case ErrorCode::PAPER_FIND_OUT_MARK: return "  Не найден конец метки";
            case ErrorCode::PAPER_PIN_NOT_CONNECT: return "  Главный мотор не подключен";
            case ErrorCode::PAPER_SENSOR_PIN_NOT_CONNECT: return "  Датчик бумаги не подключен сенсор";
            default: return "";
        }
    }

    static String TimeoutName(ErrorCode code) {
        switch (code) {
            case ErrorCode::PAPER_NOT_FOUND: return "PAPER_NOT_FOUND";
            case ErrorCode::PAPER_JAM: return "PAPER_JAM";
            case ErrorCode::PAPER_SEARCH: return "PAPER_SEARCH";
            case ErrorCode::PAPER_FIND_IN_MARK: return "PAPER_FIND_IN_MARK";
            case ErrorCode::PAPER_FIND_OUT_MARK: return "PAPER_FIND_OUT_MARK";
            case ErrorCode::TABLE_NOT_OUT: return "TABLE_NOT_OUT";
            case ErrorCode::TABLE_NOT_IN: return "TABLE_NOT_IN";
            case ErrorCode::TABLE_NOT_UP: return "TABLE_NOT_UP";
            case ErrorCode::TABLE_NOT_DOWN: return "TABLE_NOT_DOWN";
            case ErrorCode::GUILLOTINE_NOT_OUT: return "GUILLOTINE_NOT_OUT";
            case ErrorCode::GUILLOTINE_NOT_IN: return "GUILLOTINE_NOT_IN";
            default: return "";
        }
    }

    static ErrorCode TimeoutCode(const String& name, ErrorCode fallback = ErrorCode::NO_ERROR) {
        if (name == "PAPER_NOT_FOUND") return ErrorCode::PAPER_NOT_FOUND;
        if (name == "PAPER_JAM") return ErrorCode::PAPER_JAM;
        if (name == "PAPER_SEARCH") return ErrorCode::PAPER_SEARCH;
        if (name == "PAPER_FIND_IN_MARK") return ErrorCode::PAPER_FIND_IN_MARK;
        if (name == "PAPER_FIND_OUT_MARK") return ErrorCode::PAPER_FIND_OUT_MARK;
        if (name == "TABLE_NOT_OUT") return ErrorCode::TABLE_NOT_OUT;
        if (name == "TABLE_NOT_IN") return ErrorCode::TABLE_NOT_IN;
        if (name == "TABLE_NOT_UP") return ErrorCode::TABLE_NOT_UP;
        if (name == "TABLE_NOT_DOWN") return ErrorCode::TABLE_NOT_DOWN;
        if (name == "GUILLOTINE_NOT_OUT") return ErrorCode::GUILLOTINE_NOT_OUT;
        if (name == "GUILLOTINE_NOT_IN") return ErrorCode::GUILLOTINE_NOT_IN;
        return fallback;
    }

    enum class DIR : uint8_t {
        Forward = 0,
        Backward = 1
    };

  static String dirName(DIR code) {
        switch(code){
            case DIR::Forward: return "Forward";
            case DIR::Backward: return "Backward";
            default: return "";
        }
    }

    enum class SPEED : uint8_t {
        Custom = 255,
        Normal = 0,
        Slow = 1,
        Fast = 2, 
        Force = 3
    };

  static String speedName(SPEED code) {
        switch(code){
            case SPEED::Normal: return "Normal";
            case SPEED::Slow : return "Slow";
            case SPEED::Fast : return "Fast";
            case SPEED::Force : return "Force";
            case SPEED::Custom : return "Custom";
            default: return "";
        }
    }

    enum class OpticalSensor : uint8_t {
        MARK = 0,
        EDGE = 1
    };

    static const char* opticalSensorName(OpticalSensor sensor) {
        switch (sensor) {
            case OpticalSensor::MARK: return "MARK";
            case OpticalSensor::EDGE: return "EDGE";
            default: return "EDGE";
        }
    }

    // Унифицированные параметры шага плана.
    // Любое поле опционально: используется только если соответствующий has* = true.
    struct WorkParam {
        bool hasDir = false;
        DIR dir = DIR::Forward;

        bool hasSteps = false;
        int32_t steps = 0;

        bool hasMm = false;
        float mm = 0.0f;

        bool hasBlocking = false;
        bool blocking = false;

        bool hasSpeed = false;
        SPEED speed = SPEED::Normal;

        bool hasTimeout = false;
        String timeout = "";

        bool hasDetectOffset = false;
        int32_t detectOffset = 0;

        bool hasOptical = false;
        OpticalSensor optical = OpticalSensor::EDGE;

        // Fluent-методы для компактной и читаемой сборки параметров шага.
        WorkParam& Dir(DIR value) {
            hasDir = true;
            dir = value;
            return *this;
        }

        WorkParam& Step(int32_t value) {
            hasSteps = true;
            steps = value;
            return *this;
        }

        WorkParam& Mm(float value) {
            hasMm = true;
            mm = value;
            return *this;
        }

        WorkParam& Block(bool value) {
            hasBlocking = true;
            blocking = value;
            return *this;
        }

        WorkParam& Speed(SPEED value) {
            hasSpeed = true;
            speed = value;
            return *this;
        }

        WorkParam& Timeout(const String& value) {
            hasTimeout = true;
            timeout = value;
            return *this;
        }

        WorkParam& DetectOffset(int32_t value) {
            hasDetectOffset = true;
            detectOffset = value;
            return *this;
        }

        WorkParam& Optical(OpticalSensor value) {
            hasOptical = true;
            optical = value;
            return *this;
        }

        void print(){
            Log::D("{ DIR: '%s', Speed: '%s', Step:'%s', Mm:'%s', Block:'%s', Timeout:'%s', DetectOffset:'%s', Optical:'%s' }",
                Catalog::dirName(dir),
                Catalog::speedName(speed),
                String(steps),
                String(mm),
                String(blocking),
                timeout,
                String(detectOffset),
                Catalog::opticalSensorName(optical)
            );
        }
        void clear(){
            hasDir = false;
            hasBlocking = false;
            hasMm = false;
            hasSpeed = false;
            hasSteps = false;
            hasTimeout = false;
            timeout = "";
            hasDetectOffset = false;
            hasOptical = false;
            detectOffset = 0;
        }
    };

    enum class StopMode : uint8_t {
        SoftStop = 0, // Плавная остановка (stopMove).
        ForceStop = 1, // Жесткая остановка (forceStop).
        NotStop = 2 // Ничего не останавливать, только проверить факт остановки.
    };

    // Результат выполнения рабочих сценариев стола (Scene::tableUp/tableDown).
    enum class TableActionResult : uint8_t {
        Started,   // Сценарий запущен, стол начал движение.
        AtLimit,   // Стол уже в целевом пределе (верх/низ), движение не запускалось.
        NoMotor,   // Мотор стола отсутствует.
        NoSensor,  // Для сценария требуется датчик стола, но он отсутствует.
        TriggerFault // Не удалось безопасно взвести MCP-trigger, поэтому запуск стола отменен.
    };

    // Результат выполнения рабочих сценариев протяжки бумаги (Scene::paperWork).
    enum class PaperActionResult : uint8_t {
        Started,              // Запущена протяжка бумаги (и THROW, если включен и доступен).
        StartedWithoutThrow,  // Бумага запущена, но THROW отсутствует физически.
        NoPaperMotor,          // Мотор бумаги отсутствует.
        Finished // если мотор запущен в режиме блокировки
    };

private:
    Catalog() = delete;
};
