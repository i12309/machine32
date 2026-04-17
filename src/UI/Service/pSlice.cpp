#include "pSlice.h"
#include "pService.h"
#include "UI/Profile/pProfileList.h"
#include "UI/Main/pINFO.h"
#include "UI/Main/pWAIT.h"
#include "State/A/Slice.h"

void pSlice::pop_bBack(void* ptr)
{
    Log::D(__func__);
    pService::getInstance().show();
}

void pSlice::pop_bListProfile(void* ptr)
{
    Log::D(__func__);
    pProfileList& UI = pProfileList::getInstance();

    UI.backPageStatus = Catalog::PageMode::pSlice;
    UI.show();
    UI.display();
}

void pSlice::pop_bSlice(void* ptr)
{
    Log::D(__func__);
    if (!Data::work.profile.valid()) {
        pINFO::showInfo("Профиль бумаги не выбран", "Выберите профиль перед разделением");
        return;
    }

    if (Data::work.profile.RATIO_mm <= 0.0f) {
        pINFO::showInfo("Ошибка профиля бумаги", "Коэф. профиля должен быть больше 0");
        return;
    }

    pSlice& UI = pSlice::getInstance();
    String countPaperText = UI.getText(UI.tCountPaper, 10);
    countPaperText.trim();

    if (!T::isStringValidInteger(countPaperText)) {
        pINFO::showInfo("Не верные параметры", "Проверьте кол-во листов", "");
        return;
    }

    const int sheetCount = atoi(countPaperText.c_str());
    if (sheetCount <= 0) {
        pINFO::showInfo("Значение должно быть больше 0", "Проверьте кол-во листов");
        return;
    }

    Slice::setSheetCount(sheetCount);
    App::plan().clear();

    pWAIT::wait("", "Разделение листов...", "", 0, []() {
        App::state()->setNexTypeState(State::Type::SLICE);// для того что бы после CHECK он смог перейти в PROFILING
        App::state()->setFactory(State::Type::CHECK);
    }, false);
}

