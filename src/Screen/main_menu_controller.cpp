#include "Screen/main_menu_controller.h"

namespace machine32::screen {

void MainMenuController::onEnter() {
    renderStatus("main menu ready");
}

bool MainMenuController::onEnvelope(const Envelope& env, const screenlib::ScreenEventContext& ctx) {
    (void)ctx;

    if (env.which_payload != Envelope_button_event_tag) {
        return false;
    }

    if (env.payload.button_event.page_id != pageId()) {
        return false;
    }

    return handleButton(env.payload.button_event.element_id);
}

bool MainMenuController::handleButton(uint32_t elementId) {
    switch (elementId) {
        case SCREEN32_ELEMENT_ID_B_MAIN_TASK:
            _selectedSection = static_cast<uint32_t>(Section::Task);
            renderStatus("task selected");
            return true;
        case SCREEN32_ELEMENT_ID_B_MAIN_PROFILE:
            _selectedSection = static_cast<uint32_t>(Section::Profile);
            renderStatus("profile selected");
            return true;
        case SCREEN32_ELEMENT_ID_B_MAIN_NET:
            _selectedSection = static_cast<uint32_t>(Section::Net);
            renderStatus("network selected");
            return true;
        case SCREEN32_ELEMENT_ID_B_MAIN_SERVICE:
            _selectedSection = static_cast<uint32_t>(Section::Service);
            renderStatus("service selected");
            return true;
        case SCREEN32_ELEMENT_ID_B_MAIN_STATS:
            _selectedSection = static_cast<uint32_t>(Section::Stats);
            renderStatus("stats selected");
            return true;
        case SCREEN32_ELEMENT_ID_B_MAIN_SUPPORT:
            _selectedSection = static_cast<uint32_t>(Section::Support);
            renderStatus("support selected");
            return true;
        default:
            return false;
    }
}

void MainMenuController::renderStatus(const char* text) {
    _screens.setText(SCREEN32_ELEMENT_ID_C_MAIN_MENU, text);
}

}  // namespace machine32::screen
