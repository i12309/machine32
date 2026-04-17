#include "pTable.h"
#include "pService.h"
#include "UI/Main/pINFO.h"

void pTable::pop_bBack(void* ptr) {
    Log::D(__func__);
    pService::getInstance().show();
}

void pTable::pop_bUpTable(void* ptr){
    Log::D(__func__);
    const auto result = App::scene().tableUp(Catalog::SPEED::Normal);
    if (result == Catalog::TableActionResult::AtLimit) {
        pINFO::showInfo("Предупреждение", "Стол достиг максимального хода");
        return;
    }
}

void pTable::pop_bDownTable(void* ptr){
    Log::D(__func__);
    const auto result = App::scene().tableDown(Catalog::SPEED::Normal);
    if (result == Catalog::TableActionResult::AtLimit) {
        pINFO::showInfo("Предупреждение", "Стол находится в нижней точке");
        return;
    }
}

void pTable::pop_bStopTBL(void* ptr){
    Log::D(__func__);
    App::scene().tableStop(Catalog::StopMode::ForceStop);
}

void pTable::push_bSlowUp(void* ptr) {
    Log::D(__func__);
    const auto result = App::scene().tableUp(Catalog::SPEED::Slow);
    if (result == Catalog::TableActionResult::AtLimit) {
        pINFO::showInfo("Предупреждение", "Стол достиг максимального хода");
        return;
    }
}

void pTable::pop_bSlowUp(void* ptr) {
    Log::D(__func__);
    App::scene().tableStop(Catalog::StopMode::ForceStop);
}

void pTable::push_bSlowDown(void* ptr) {
    Log::D(__func__);
    const auto result = App::scene().tableDown(Catalog::SPEED::Slow);
    if (result == Catalog::TableActionResult::AtLimit) {
        pINFO::showInfo("Предупреждение", "Стол находится в нижней точке");
        return;
    }
}

void pTable::pop_bSlowDown(void* ptr) {
    Log::D(__func__);
    App::scene().tableStop(Catalog::StopMode::ForceStop);
}

