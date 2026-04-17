#pragma once

#include "Machine/Context/IMachineContext.h"

class MachineContextB : public IMachineContext {
public:
    static MachineContextB& instance();

    Catalog::MachineType type() const override { return Catalog::MachineType::B; }
    void bind(Registry& registry) override;
    bool readyForMotion() const override;
    void collectIssues(std::vector<String>& issues) const override;
};
