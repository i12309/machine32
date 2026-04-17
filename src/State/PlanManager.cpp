#include "PlanManager.h"

#include "Core.h"

PlanManager& PlanManager::instance() {
    static PlanManager manager;
    return manager;
}

void PlanManager::beginPlan(Type ownerType) {
    clear();
    this->ownerType = ownerType;
}

void PlanManager::add(Type stateType, const Catalog::WorkParam& params, bool alreadyCalled) {
    addAction(stateType, Action(), "", params, alreadyCalled);
}

void PlanManager::addAction(Type stateType, Action action, std::string actionName, const Catalog::WorkParam& params, bool alreadyCalled) {
    PlanItem item;
    item.stateType = stateType;
    item.called = alreadyCalled;
    item.action = action;
    item.actionName = actionName;
    // Параметры шага сохраняем вместе с самим шагом плана.
    item.params = params;
    planItems.push_back(item);
}

void PlanManager::clear() {
    planItems.clear();
    ownerType = Type::NULL_STATE;
    returnToType = Type::NULL_STATE;
    lastReturnedType = Type::NULL_STATE;
    lastReturnedIndex = -1;
    currentAction = Action();
    currentActionName = "";
    currentParams = Catalog::WorkParam();
}

bool PlanManager::hasPending() const {
    for (const auto& item : planItems) {
        if (!item.called) {
            return true;
        }
    }
    return false;
}

PlanManager::Type PlanManager::nextType(Type callerType) {
    if (ownerType == Type::NULL_STATE) {
        ownerType = callerType;
    }
    if (ownerType != callerType) {
        Log::E("Plan owner mismatch: owner=%s caller=%s",
               Catalog::getStateName(ownerType).c_str(),
               Catalog::getStateName(callerType).c_str());
        printPlan();
        clear();
        //TODO - setError(State::ErrorCode::PAPER_JAM);
        return Type::ERROR;
    }

    returnToType = callerType;

    for (size_t i = 0; i < planItems.size(); i++) {
        auto& item = planItems[i];
        if (!item.called) {
            item.called = true;
            lastReturnedType = item.stateType;
            lastReturnedIndex = static_cast<int>(i);
            currentAction = item.action;
            currentActionName = item.actionName;
            // Сохраняем параметры именно того шага, который сейчас запускается.
            currentParams = item.params;
            return item.stateType;
        }
    }

    clear();
    return Type::NULL_STATE;
}

PlanManager::Type PlanManager::lastType() const {
    return lastReturnedType;
}

int PlanManager::lastIndex() const {
    return lastReturnedIndex;
}

void PlanManager::resetByType(Type stateType) {
    for (auto& item : planItems) {
        if (item.stateType == stateType) {
            item.called = false;
        }
    }
}

void PlanManager::resetByIndex(int index) {
    if (index < 0 || index >= static_cast<int>(planItems.size())) {
        return;
    }
    planItems[index].called = false;
}

void PlanManager::resetByActionName(std::string actionName) {
    for (auto& item : planItems) {
        if (item.actionName == actionName) {
            item.called = false;
        }
    }
}

void PlanManager::printPlan() const {
    Log::D("=== PLAN ===");
    for (int i = 0; i < planItems.size(); i++) {
        std::string name = planItems[i].actionName.empty() ? "" : " (" + planItems[i].actionName + ")";
        Log::D("[%d] %s - %s%s", i, Catalog::getStateName(planItems[i].stateType).c_str(),
               planItems[i].called ? "DONE" : "PENDING", name.c_str());
    }
    Log::D("==========");
}

bool PlanManager::hasReturnTarget() const {
    return returnToType != Type::NULL_STATE;
}

PlanManager::Type PlanManager::consumeReturnTarget() {
    Type target = returnToType;
    returnToType = Type::NULL_STATE;
    return target;
}

PlanManager::Action PlanManager::consumeAction() {
    Action action = currentAction;
    currentAction = Action();
    return action;
}

std::string PlanManager::consumeActionName() {
    std::string name = currentActionName;
    if (!name.empty()) lastActionName = name;
    currentActionName = "";
    return name;
}

std::string PlanManager::getLastActionName() {
    return lastActionName;
}

Catalog::WorkParam PlanManager::getCurrentParams() const {
    return currentParams;
}

bool PlanManager::isActive() const {
    return ownerType != Type::NULL_STATE;
}

PlanManager::Type PlanManager::owner() const {
    return ownerType;
}
