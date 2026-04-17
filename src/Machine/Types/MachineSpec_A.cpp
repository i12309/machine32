#include "Machine/MachineSpec.h"

MachineSpec MachineSpec::makeA() {
    MachineSpec spec;
    spec.type_ = Catalog::MachineType::A;

    // I2C расширители.
    spec.i2c_.push_back({ "MCP0", true });
    spec.i2c_.push_back({ "MCP1", true });
    spec.i2c_.push_back({ "MCP2", true });

    // Моторы.
    spec.motors_.push_back({ "TABLE", true });
    spec.motors_.push_back({ "PAPER", true });
    spec.motors_.push_back({ "GUILLOTINE", true });

    // Датчики.
    spec.sensors_.push_back({ "TABLE_UP", true });
    spec.sensors_.push_back({ "TABLE_DOWN", true });
    spec.sensors_.push_back({ "THROW", true });
    spec.sensors_.push_back({ "GUILLOTINE", true });

    // Оптика.
    spec.optical_.push_back({ "MARK", true });
    spec.optical_.push_back({ "EDGE", true });

    // Переключатели.
    spec.switchs_.push_back({ "CATCH", true });
    spec.switchs_.push_back({ "THROW", true });
    spec.switchs_.push_back({ "PAPER", true });

    // Кнопки.
    spec.buttons_.push_back({ "START", false });

    // Энкодеры.
    spec.encoders_.push_back({ "CATCH", true });

    return spec;
}

void MachineSpec::fillDefaultsA(JsonObject device) const {
    // Полный дефолт для типа A.
    // Заполняем не только имена узлов, но и все ключевые атрибуты,
    // чтобы картина device была целостной и готовой к работе без ручных дописок.

    JsonObject i2c = device["I2C"].to<JsonObject>();
    {
        JsonObject mcp0 = i2c["MCP0"].to<JsonObject>();
        mcp0["type"] = "output";
        mcp0["address"] = "0x20";

        JsonObject mcp1 = i2c["MCP1"].to<JsonObject>();
        mcp1["type"] = "output";
        mcp1["address"] = "0x21";

        JsonObject mcp2 = i2c["MCP2"].to<JsonObject>();
        mcp2["type"] = "input";
        mcp2["address"] = "0x22";
        mcp2["intA"] = 39;
        mcp2["intB"] = 34;
    }

    JsonObject motors = device["motors"].to<JsonObject>();
    {
        JsonObject table = motors["TABLE"].to<JsonObject>();
        table["I2C"] = "MCP0";
        JsonObject tablePin = table["pin"].to<JsonObject>();
        tablePin["step"] = 23;
        tablePin["dir"] = 0;
        tablePin["ena"] = 1;
        table["MicroStep"] = 3200;
        table["WorkMove"] = 9500;
        JsonArray tableSpeed = table["Speed"].to<JsonArray>();
        JsonObject tableSpeedNormal = tableSpeed.add<JsonObject>();
        tableSpeedNormal["Mode"] = "Normal";
        tableSpeedNormal["Speed"] = 16000;
        tableSpeedNormal["Acceleration"] = 32000;
        JsonObject tableSpeedSlow = tableSpeed.add<JsonObject>();
        tableSpeedSlow["Mode"] = "Slow";
        tableSpeedSlow["Speed"] = 9600;
        tableSpeedSlow["Acceleration"] = 9600;
        JsonArray tableCheck = table["Check"].to<JsonArray>();
        JsonObject tableCheckInDown = tableCheck.add<JsonObject>();
        tableCheckInDown["Mode"] = "TABLE_NOT_DOWN";
        tableCheckInDown["step"] = 19000;
        tableCheckInDown["ms"] = -1;
        JsonObject tableCheckInUp = tableCheck.add<JsonObject>();
        tableCheckInUp["Mode"] = "TABLE_NOT_UP";
        tableCheckInUp["step"] = 19000;
        tableCheckInUp["ms"] = -1;

        JsonObject paper = motors["PAPER"].to<JsonObject>();
        paper["I2C"] = "MCP0";
        JsonObject paperPin = paper["pin"].to<JsonObject>();
        paperPin["step"] = 19;
        paperPin["dir"] = 2;
        paperPin["ena"] = 3;
        paper["MicroStep"] = 3200;
        paper["WorkMove"] = 3200;
        JsonArray paperSpeed = paper["Speed"].to<JsonArray>();
        JsonObject paperSpeedNormal = paperSpeed.add<JsonObject>();
        paperSpeedNormal["Mode"] = "Normal";
        paperSpeedNormal["Speed"] = 16000;
        paperSpeedNormal["Acceleration"] = 32000;
        JsonObject paperSpeedSlow = paperSpeed.add<JsonObject>();
        paperSpeedSlow["Mode"] = "Slow";
        paperSpeedSlow["Speed"] = 9600;
        paperSpeedSlow["Acceleration"] = 9600;
        JsonObject paperSpeedPaper = paperSpeed.add<JsonObject>();
        paperSpeedPaper["Mode"] = "Paper";
        paperSpeedPaper["Speed"] = 4800;
        paperSpeedPaper["Acceleration"] = 32000;
        JsonObject paperSpeedMark = paperSpeed.add<JsonObject>();
        paperSpeedMark["Mode"] = "Mark";
        paperSpeedMark["Speed"] = 2400;
        paperSpeedMark["Acceleration"] = 32000;
        JsonObject paperSpeedForce = paperSpeed.add<JsonObject>();
        paperSpeedForce["Mode"] = "Force";
        paperSpeedForce["Speed"] = 32000;
        paperSpeedForce["Acceleration"] = 32000;

        JsonArray paperCheck = paper["Check"].to<JsonArray>();
        JsonObject paperCheckNotFound = paperCheck.add<JsonObject>();
        paperCheckNotFound["Mode"] = "PAPER_NOT_FOUND";
        paperCheckNotFound["step"] = 7200;
        paperCheckNotFound["ms"] = -1;
        JsonObject paperCheckFindIn = paperCheck.add<JsonObject>();
        paperCheckFindIn["Mode"] = "PAPER_FIND_IN_MARK";
        paperCheckFindIn["step"] = 1000;
        paperCheckFindIn["ms"] = -1;
        JsonObject paperCheckFindOut = paperCheck.add<JsonObject>();
        paperCheckFindOut["Mode"] = "PAPER_FIND_OUT_MARK";
        paperCheckFindOut["step"] = 1000;
        paperCheckFindOut["ms"] = -1;
        JsonObject paperCheckSearch = paperCheck.add<JsonObject>();
        paperCheckSearch["Mode"] = "PAPER_SEARCH";
        paperCheckSearch["step"] = 30000;
        paperCheckSearch["ms"] = -1;

        JsonObject guillotine = motors["GUILLOTINE"].to<JsonObject>();
        guillotine["I2C"] = "MCP0";
        JsonObject guillotinePin = guillotine["pin"].to<JsonObject>();
        guillotinePin["step"] = 18;
        guillotinePin["dir"] = 4;
        guillotinePin["ena"] = 5;
        guillotine["MicroStep"] = 3200;
        guillotine["WorkMove"] = 3200;
        JsonArray guillotineSpeed = guillotine["Speed"].to<JsonArray>();
        JsonObject guillotineSpeedNormal = guillotineSpeed.add<JsonObject>();
        guillotineSpeedNormal["Mode"] = "Normal";
        guillotineSpeedNormal["Speed"] = 16000;
        guillotineSpeedNormal["Acceleration"] = 32000;
        JsonObject guillotineSpeedSlow = guillotineSpeed.add<JsonObject>();
        guillotineSpeedSlow["Mode"] = "Slow";
        guillotineSpeedSlow["Speed"] = 9600;
        guillotineSpeedSlow["Acceleration"] = 9600;
        JsonArray guillotineCheck = guillotine["Check"].to<JsonArray>();
        JsonObject guillotineCheckNotIn = guillotineCheck.add<JsonObject>();
        guillotineCheckNotIn["Mode"] = "GUILLOTINE_NOT_IN";
        guillotineCheckNotIn["step"] = 4000;
        guillotineCheckNotIn["ms"] = 1500;
    }

    JsonObject sensors = device["sensors"].to<JsonObject>();
    {
        JsonObject tableUp = sensors["TABLE_UP"].to<JsonObject>();
        tableUp["I2C"] = "MCP2";
        tableUp["pin"] = 0;

        JsonObject tableDown = sensors["TABLE_DOWN"].to<JsonObject>();
        tableDown["I2C"] = "MCP2";
        tableDown["pin"] = 1;

        JsonObject throwSensor = sensors["THROW"].to<JsonObject>();
        throwSensor["I2C"] = "MCP2";
        throwSensor["pin"] = 3;

        JsonObject guillotine = sensors["GUILLOTINE"].to<JsonObject>();
        guillotine["I2C"] = "MCP2";
        guillotine["pin"] = 2;
    }

    JsonObject optical = device["optical"].to<JsonObject>();
    {
        JsonObject mark = optical["MARK"].to<JsonObject>();
        mark["pin"] = 35;
        mark["sig"] = 1;

        JsonObject edge = optical["EDGE"].to<JsonObject>();
        edge["pin"] = 36;
        edge["sig"] = 0;
    }

    JsonObject switchs = device["switchs"].to<JsonObject>();
    {
        JsonObject catchSwitch = switchs["CATCH"].to<JsonObject>();
        catchSwitch["I2C"] = "MCP1";
        catchSwitch["pin"] = 12;
        catchSwitch["sig"] = 0;

        JsonObject throwSwitch = switchs["THROW"].to<JsonObject>();
        throwSwitch["I2C"] = "MCP1";
        throwSwitch["pin"] = 13;
        throwSwitch["sig"] = 0;

        JsonObject paperSwitch = switchs["PAPER"].to<JsonObject>();
        paperSwitch["I2C"] = "MCP1";
        paperSwitch["pin"] = 14;
        paperSwitch["sig"] = 0;
    }

    JsonObject buttons = device["buttons"].to<JsonObject>();
    {
        JsonObject startBtn = buttons["START"].to<JsonObject>();
        startBtn["I2C"] = "MCP2";
        startBtn["pin"] = 16;
    }

    JsonObject encoders = device["encoders"].to<JsonObject>();
    {
        JsonObject catchEncoder = encoders["CATCH"].to<JsonObject>();
        catchEncoder["I2C"] = "ESP32";
        catchEncoder["pinA"] = 4;
        catchEncoder["pinB"] = 5;
        catchEncoder["pcnt"] = 6;
        catchEncoder["threshold"] = 100;
    }
}
