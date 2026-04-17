#pragma once

#include <stdint.h>

#include "generated/shared/element_ids.generated.h"
#include "generated/shared/page_ids.generated.h"
#include "pages/IPageController.h"
#include "system/ScreenSystem.h"

namespace machine32::screen {

class MainMenuController : public screenlib::IPageController {
public:
    explicit MainMenuController(screenlib::ScreenSystem& screens) : _screens(screens) {}

    uint32_t pageId() const override { return SCREEN32_PAGE_ID_MAIN_MENU; }
    void onEnter() override;
    bool onEnvelope(const Envelope& env, const screenlib::ScreenEventContext& ctx) override;

    uint32_t selectedSection() const { return _selectedSection; }

private:
    enum class Section : uint32_t {
        None = 0,
        Task,
        Profile,
        Net,
        Service,
        Stats,
        Support
    };

    bool handleButton(uint32_t elementId);
    void renderStatus(const char* text);

    screenlib::ScreenSystem& _screens;
    uint32_t _selectedSection = static_cast<uint32_t>(Section::None);
};

}  // namespace machine32::screen
