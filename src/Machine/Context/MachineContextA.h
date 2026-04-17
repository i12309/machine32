#pragma once

#include "Machine/Context/IMachineContext.h"

class McpTrigger;

class MachineContextA : public IMachineContext {
public:
    static MachineContextA& instance();

    Catalog::MachineType type() const override { return Catalog::MachineType::A; }
    void bind(Registry& registry) override;
    void load();
    bool readyForMotion() const override;
    void collectIssues(std::vector<String>& issues) const override;
};
