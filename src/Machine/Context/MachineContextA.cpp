#include "Machine/Context/MachineContextA.h"

namespace {

void addIssue(std::vector<String>& issues, const char* name, bool missing) {
    if (!missing) return;
    issues.push_back(String("[MachineContextA] Missing required object: ") + name);
}

} // namespace

MachineContextA& MachineContextA::instance() {
    static MachineContextA ctx;
    return ctx;
}

void MachineContextA::bind(Registry& registry) {
    mPaper = registry.getMotor("PAPER");
    eCatch = registry.getEncoder("CATCH");
    ePaper = registry.getEncoder("PAPER");
    //cPaper = registry.getClutch("PAPER");
    swCatch = registry.getSwitch("CATCH");
    swThrow = registry.getSwitch("THROW");
    swPaper = registry.getSwitch("PAPER");
    oMark = registry.getOptical("MARK");
    oEdge = registry.getOptical("EDGE");

    mTable = registry.getMotor("TABLE");
    sTableUp = registry.getSensor("TABLE_UP");
    sTableDown = registry.getSensor("TABLE_DOWN");

    mGuillotine = registry.getMotor("GUILLOTINE");
    sGuillotine = registry.getSensor("GUILLOTINE");

    sThrow = registry.getSensor("THROW");

    mBigelUp = registry.getMotor("BIGEL_UP");
    mBigelDown = registry.getMotor("BIGEL_DOWN");

    mKnife1 = registry.getMotor("KNIFE1");
}

void MachineContextA::load() {
    bind(reg);
}

bool MachineContextA::readyForMotion() const {
    return mPaper != nullptr &&
           eCatch != nullptr &&
           swCatch != nullptr &&
           swThrow != nullptr &&
           swPaper != nullptr &&
           oMark != nullptr &&
           oEdge != nullptr &&
           mTable != nullptr &&
           sTableUp != nullptr &&
           sTableDown != nullptr &&
           mGuillotine != nullptr &&
           sGuillotine != nullptr &&
           sThrow != nullptr;
}

void MachineContextA::collectIssues(std::vector<String>& issues) const {
    addIssue(issues, "motors.PAPER", mPaper == nullptr);
    addIssue(issues, "encoders.CATCH", eCatch == nullptr);
    addIssue(issues, "switchs.CATCH", swCatch == nullptr);
    addIssue(issues, "switchs.THROW", swThrow == nullptr);
    addIssue(issues, "switchs.PAPER", swPaper == nullptr);
    addIssue(issues, "optical.MARK", oMark == nullptr);
    addIssue(issues, "optical.EDGE", oEdge == nullptr);

    addIssue(issues, "motors.TABLE", mTable == nullptr);
    addIssue(issues, "sensors.TABLE_UP", sTableUp == nullptr);
    addIssue(issues, "sensors.TABLE_DOWN", sTableDown == nullptr);

    addIssue(issues, "motors.GUILLOTINE", mGuillotine == nullptr);
    addIssue(issues, "sensors.GUILLOTINE", sGuillotine == nullptr);

    addIssue(issues, "sensors.THROW", sThrow == nullptr);
}
