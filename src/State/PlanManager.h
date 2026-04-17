#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Catalog.h"

class PlanManager {
public:
    using Type = Catalog::State;
    using Action = std::function<bool()>;

    struct PlanItem {
        Type stateType;
        bool called;
        Action action;
        std::string actionName;
        // Опциональные параметры конкретного шага (направление/шаги/мм/блокировка).
        Catalog::WorkParam params;
    };

    static PlanManager& instance();

    void beginPlan(Type ownerType);
    void add(Type stateType, const Catalog::WorkParam& params = Catalog::WorkParam(), bool alreadyCalled = false);
    void addAction(Type stateType, Action action, std::string actionName = "", const Catalog::WorkParam& params = Catalog::WorkParam(), bool alreadyCalled = false);
    void clear();

    bool hasPending() const;
    Type nextType(Type callerType);
    Type lastType() const;
    int lastIndex() const;

    void resetByType(Type stateType);
    void resetByIndex(int index);
    void resetByActionName(std::string actionName);

    void printPlan() const;

    bool hasReturnTarget() const;
    Type consumeReturnTarget();
    Action consumeAction();
    std::string consumeActionName();
    std::string getLastActionName();
    // Параметры шага, который был выдан последним nextType().
    Catalog::WorkParam getCurrentParams() const;

    bool isActive() const;
    Type owner() const;

private:
    PlanManager() = default;

    std::vector<PlanItem> planItems;
    Type ownerType = Type::NULL_STATE;
    Type returnToType = Type::NULL_STATE;
    Type lastReturnedType = Type::NULL_STATE;
    int lastReturnedIndex = -1;
    Action currentAction;
    std::string currentActionName;
    std::string lastActionName;
    Catalog::WorkParam currentParams;
};
