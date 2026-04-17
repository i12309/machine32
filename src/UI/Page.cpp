#include "Page.h"

#include "UI/Main/pERROR.h"
#include "UI/Main/pINFO.h"

namespace {
bool warningDialogOpened = false;
}

const char* Page::_func = "";
Page* Page::activePage = nullptr;  // Определение статической переменной
Page* Page::previousPage = nullptr;

void Page::process() {
    processWarnings();
    if (Page::activePage) Page::activePage->loop();
}

void Page::processWarnings() {
    DeviceError& diag = App::diag();

    if (!diag.hasWarnings()) {
        warningDialogOpened = false;
        return;
    }

    if (diag.isCollecting()) return;
    if (warningDialogOpened) return;

    if (Page::activePage == &pINFO::getInstance() || Page::activePage == &pERROR::getInstance()) {
        return;
    }

    warningDialogOpened = true;
    pINFO::showInfo(
        "Есть предупреждения",
        "Показать список диагностики?",
        "",
        []() {
            warningDialogOpened = false;
            pERROR::getInstance().show();
        },
        []() {
            App::diag().clearWarnings();
            warningDialogOpened = false;
            pINFO::getInstance().back();
        },
        true,
        "Показать",
        "Сбросить"
    );
}
