#include "pThrow.h"
#include "pService2.h"

void pThrow::pop_bBack(void* ptr)
{
    Log::D(__func__);
    pThrow& UI = pThrow::getInstance();
    UI.stopWork(Catalog::StopMode::ForceStop);
    pService2::getInstance().show();
}

void pThrow::pop_bStop(void* ptr)
{
    Log::D(__func__);
    pThrow& UI = pThrow::getInstance();
    UI.stopWork(Catalog::StopMode::ForceStop);
    UI.setClutchUi(UI.clutch_switch);
    UI.setPaperUi(UI.paper_switch);
}

void pThrow::pop_bClutch(void* ptr)
{
    Log::D(__func__);
    pThrow& UI = pThrow::getInstance();
    if (App::ctx().swCatch == nullptr) return;

    UI.clutch_switch = !UI.clutch_switch;
    UI.setClutchUi(UI.clutch_switch);
}

void pThrow::pop_bPaper(void* ptr)
{
    Log::D(__func__);
    pThrow& UI = pThrow::getInstance();
    if (App::ctx().mPaper == nullptr || App::ctx().swPaper == nullptr) return;

    UI.paper_switch = !UI.paper_switch;
    if (UI.paper_switch) App::ctx().swPaper->on();
    else App::ctx().swPaper->off();
    UI.setPaperUi(UI.paper_switch);
}

void pThrow::push_bFwd(void* ptr) {
    Log::D(__func__);
    pThrow& UI = pThrow::getInstance();
    UI.startWork(Catalog::DIR::Forward);
}

void pThrow::pop_bFwd(void* ptr) {
    Log::D(__func__);
    pThrow& UI = pThrow::getInstance();
    UI.stopWork(Catalog::StopMode::SoftStop);
}

void pThrow::push_bBkw(void* ptr) {
    Log::D(__func__);
    pThrow& UI = pThrow::getInstance();
    UI.startWork(Catalog::DIR::Backward);
}

void pThrow::pop_bBkw(void* ptr) {
    Log::D(__func__);
    pThrow& UI = pThrow::getInstance();
    UI.stopWork(Catalog::StopMode::SoftStop);
}
