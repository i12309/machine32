#include "pPaper.h"
#include "pService.h"
#include "UI/Main/pINFO.h"

void pPaper::pop_bBack(void* ptr) {
    Log::D(__func__);
    App::scene().paperStop(Catalog::StopMode::ForceStop);
    pPaper& UI = pPaper::getInstance();
    pService::getInstance().show();
}

void pPaper::pop_bDetectPaper(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    //App::scene().paperStop(Catalog::StopMode::ForceStop);
    App::scene().paperDetectPaper(UI.throw_switch, UI.detectDir, Catalog::SPEED::Custom, UI.clutch_switch, 0, UI.opticalSensor);
}

void pPaper::pop_bDetectMark(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    //App::scene().paperStop(Catalog::StopMode::ForceStop);
    App::scene().paperDetectMark(UI.throw_switch, UI.detectDir, Catalog::SPEED::Custom, UI.clutch_switch, 0, UI.opticalSensor);
}

void pPaper::pop_bClutch(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    UI.clutch_switch = !UI.clutch_switch;
    UI.setClutchUi(UI.clutch_switch);
}

void pPaper::pop_bThrow(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    UI.throw_switch = !UI.throw_switch;
    UI.setThrowUi(UI.throw_switch);
}

void pPaper::pop_bStop(void* ptr) {
    Log::D(__func__);
    App::scene().paperStop(Catalog::StopMode::ForceStop);
}

void pPaper::push_bPForward(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    int v = UI.normalizeStepValueUi();
    if (v==0) App::scene().paperWork(Catalog::DIR::Forward, Catalog::SPEED::Slow, UI.throw_switch, UI.clutch_switch);
    else App::scene().paperMove(v, Catalog::DIR::Forward, Catalog::SPEED::Slow, true, UI.clutch_switch, UI.throw_switch);
}

void pPaper::pop_bPForward(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    if (UI.normalizeStepValueUi()==0) App::scene().paperStop(Catalog::StopMode::SoftStop);
}

void pPaper::push_bPBack(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    int v = UI.normalizeStepValueUi();
    if (v==0) App::scene().paperWork(Catalog::DIR::Backward, Catalog::SPEED::Slow, UI.throw_switch, UI.clutch_switch);
    else App::scene().paperMove(v, Catalog::DIR::Backward, Catalog::SPEED::Slow, true, UI.clutch_switch, UI.throw_switch);
}

void pPaper::pop_bPBack(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    if (UI.normalizeStepValueUi()==0) App::scene().paperStop(Catalog::StopMode::SoftStop);
}

void pPaper::pop_bBwd(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    UI.detectDir = Catalog::DIR::Backward;
    UI.setDirUi();
}
void pPaper::pop_bFwd(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    UI.detectDir = Catalog::DIR::Forward;
    UI.setDirUi();
}
void pPaper::pop_bMark(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    UI.opticalSensor = Catalog::OpticalSensor::MARK;
    UI.setOpticalUi();
}
void pPaper::pop_bEdge(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    UI.opticalSensor = Catalog::OpticalSensor::EDGE;
    UI.setOpticalUi();
}
void pPaper::pop_bStepMM(void* ptr) {
    Log::D(__func__);
}

void pPaper::pop_cCheck(void* ptr) {
    Log::D(__func__);
    App::scene().guillotineWork(Catalog::DIR::Forward,350, Catalog::SPEED::Normal);
}

void pPaper::pop_tEncoder(void* ptr) {
    Log::D(__func__);
    pPaper& UI = pPaper::getInstance();
    App::ctx().ePaper->clearCount();
    App::ctx().mPaper->setCurrentPosition(0); // обнуляем мотор 
    String text = String(App::ctx().ePaper->getCount());
    UI.tEncoder.setText(text.c_str());
}
