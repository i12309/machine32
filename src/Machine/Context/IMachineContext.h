#pragma once

#include <Arduino.h>
#include <vector>

#include "Catalog.h"
#include "Controller/Registry.h"
#include "Controller/McpTrigger.h"

// Базовый интерфейс machine-specific контекста.
// Context ничего не создает, а только связывается с тем,
// что уже зарегистрировано в Registry.
class IMachineContext {
public:
    Registry& reg;
    McpTrigger& mcpTrigger;

    IStepper* mPaper = nullptr;
    IEncoder* eCatch = nullptr;
    IEncoder* ePaper = nullptr;
    IClutch* cPaper = nullptr;
    ISwitch* swCatch = nullptr;
    ISwitch* swThrow = nullptr;
    ISwitch* swPaper = nullptr;
    IOptical* oMark = nullptr;
    IOptical* oEdge = nullptr;

    IStepper* mTable = nullptr;
    ISensor* sTableUp = nullptr;
    ISensor* sTableDown = nullptr;

    IStepper* mGuillotine = nullptr;
    ISensor* sGuillotine = nullptr;

    ISensor* sThrow = nullptr;

    IStepper* mBigelUp = nullptr;
    IStepper* mBigelDown = nullptr;

    IStepper* mKnife1 = nullptr;

    virtual ~IMachineContext() = default;

    virtual Catalog::MachineType type() const { return Catalog::MachineType::UNKNOWN; }
    virtual void bind(Registry& registry) { (void)registry; }
    virtual bool readyForMotion() const { return false; }
    virtual void collectIssues(std::vector<String>& issues) const { (void)issues; }

    void clearBindings() {
        mPaper = nullptr;
        eCatch = nullptr;
        ePaper = nullptr;
        cPaper = nullptr;
        swCatch = nullptr;
        swThrow = nullptr;
        swPaper = nullptr;
        oMark = nullptr;
        oEdge = nullptr;
        mTable = nullptr;
        sTableUp = nullptr;
        sTableDown = nullptr;
        mGuillotine = nullptr;
        sGuillotine = nullptr;
        sThrow = nullptr;
        mBigelUp = nullptr;
        mBigelDown = nullptr;
        mKnife1 = nullptr;
    }

    void copyBindingsFrom(const IMachineContext& other) {
        mPaper = other.mPaper;
        eCatch = other.eCatch;
        ePaper = other.ePaper;
        cPaper = other.cPaper;
        swCatch = other.swCatch;
        swThrow = other.swThrow;
        swPaper = other.swPaper;
        oMark = other.oMark;
        oEdge = other.oEdge;
        mTable = other.mTable;
        sTableUp = other.sTableUp;
        sTableDown = other.sTableDown;
        mGuillotine = other.mGuillotine;
        sGuillotine = other.sGuillotine;
        sThrow = other.sThrow;
        mBigelUp = other.mBigelUp;
        mBigelDown = other.mBigelDown;
        mKnife1 = other.mKnife1;
    }

    IMachineContext()
        : reg(Registry::getInstance())
        , mcpTrigger(McpTrigger::getInstance()) {}
};
