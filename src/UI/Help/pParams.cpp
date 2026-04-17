#include "pParams.h"
#include "pDevice.h"

void pParams::pop_bBack(void* ptr){
    Log::D(__func__);
    pDevice::getInstance().show();
}

void pParams::pop_bSave(void* ptr){
    Log::D(__func__);
    Core::settings.ACCESS_POINT = pParams::getInstance().getCBvalue(pParams::getInstance().cAccessPoint);
    pParams& UI = pParams::getInstance();
    Core::config.machine = UI.getText(UI.tMachine,32);
    Core::config.group = UI.getText(UI.tGroup,32);
    Core::config.name = UI.getText(UI.tName,32);
    Core::config.save();
    pDevice::getInstance().show();
}