#pragma once

#include "Screen/Screen.h"
#include "backend_pages/init_base.h"

namespace machine32::screen {

// Экран инициализации в новом runtime. Поля и события соответствуют
// regenerated INIT из ScreenUI и по смыслу повторяют старый pINIT.
class Init : public screenui::InitBase<Init> {
protected:
    void onShow() override;

    void onClickInitHttp() override;
    void onClickInitOk() override;
    void onClickInitGroup() override;
    void onClickInitName() override;
    void onClickInitAccessPoint() override;
    void onClickInitTest() override;

    void onChangeInitMachine(int32_t value) override;
    void onChangeInitRAccessPoint(int32_t value) override;
    void onChangeInitRTest(int32_t value) override;
};

}  // namespace machine32::screen
