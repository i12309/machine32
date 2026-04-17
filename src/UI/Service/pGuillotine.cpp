#include "pGuillotine.h"
#include "pService.h"
#include "State/Scene.h"

void pGuillotine::pop_bBack(void *ptr)
{
    Log::D(__func__);
    App::scene().guillotineStop(Catalog::StopMode::ForceStop);
    pService::getInstance().show();
}

void pGuillotine::pop_bGuillotineFwd(void *ptr)
{
    Log::D(__func__);
    pGuillotine& UI = pGuillotine::getInstance();
    App::scene().guillotineWork(Catalog::DIR::Forward, 350, Catalog::SPEED::Normal, DeviceError::Kind::Fatal, UI.throw_switch);
}

void pGuillotine::pop_bGuillotineBkw(void *ptr)
{
    Log::D(__func__);
    pGuillotine& UI = pGuillotine::getInstance();
    App::scene().guillotineWork(Catalog::DIR::Backward, 350, Catalog::SPEED::Normal, DeviceError::Kind::Fatal, UI.throw_switch);
}

void pGuillotine::push_bSlowFwd(void* ptr) {
    Log::D(__func__);
    pGuillotine& UI = pGuillotine::getInstance();
    App::scene().guillotineWork(Catalog::DIR::Forward, 0, Catalog::SPEED::Slow, DeviceError::Kind::Error, UI.throw_switch);
}

void pGuillotine::pop_bSlowFwd(void* ptr) {
    Log::D(__func__);
    App::scene().guillotineStop(Catalog::StopMode::ForceStop);
}

void pGuillotine::push_bSlowBkw(void* ptr) {
    Log::D(__func__);
    pGuillotine& UI = pGuillotine::getInstance();
    App::scene().guillotineWork(Catalog::DIR::Backward, 0, Catalog::SPEED::Slow, DeviceError::Kind::Error, UI.throw_switch);
}

void pGuillotine::pop_bSlowBkw(void* ptr) {
    Log::D(__func__);
    App::scene().guillotineStop(Catalog::StopMode::ForceStop);
}

void pGuillotine::pop_bStopGLT(void *ptr)
{
    Log::D(__func__);
    App::scene().guillotineStop(Catalog::StopMode::ForceStop);
}

void pGuillotine::pop_bThrow(void* ptr) {
    Log::D(__func__);
    pGuillotine& UI = pGuillotine::getInstance();
    if (App::ctx().swThrow == nullptr) return;

    UI.throw_switch = !UI.throw_switch;
    UI.setThrowUi();
}
