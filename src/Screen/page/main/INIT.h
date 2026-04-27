#pragma once

#include "Screen/Screen.h"
#include "backend_pages/init_base.h"

namespace machine32::screen {

class Init : public screenui::InitPage<Init> {
public:
    static bool show() {
        return Screen::getInstance().showPage<Init>();
    }

protected:
    void onShow() override;
    void onInputText(uint32_t elementId, const char* text) override;
    void onInputInt(uint32_t elementId, int32_t value) override;

private:
    void handleHttp();
    void handleOk();
    void handleGroup();
    void handleName();
    void handleAccessPoint();
    void handleTest();

    void handleMachineChange(int32_t value);
    void handleGroupChange(const char* text);
    void handleNameChange(const char* text);
    void handleAccessPointChange(int32_t value);
    void handleTestChange(int32_t value);
};

}  // namespace machine32::screen
