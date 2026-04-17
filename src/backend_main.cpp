#include <cstdio>

#include "Screen/main_menu_controller.h"
#include "Screen/runtime.h"
#include "generated/shared/element_descriptors.generated.h"

int main() {
    screenlib::ScreenConfig config{};

    machine32::screen::runtime runtime;
    machine32::screen::MainMenuController mainMenuController(runtime.screens());
    runtime.addPageController(&mainMenuController);

    if (!runtime.init(config)) {
        std::printf("machine32 screen prep init failed: %s\n", runtime.lastError());
        return 1;
    }

    runtime.selectPage(SCREEN32_PAGE_ID_MAIN_MENU);

    Envelope env = Envelope_init_zero;
    env.which_payload = Envelope_button_event_tag;
    env.payload.button_event.page_id = SCREEN32_PAGE_ID_MAIN_MENU;
    env.payload.button_event.element_id = SCREEN32_ELEMENT_ID_B_MAIN_TASK;
    runtime.dispatchEnvelopeForTest(env);

    const machine32::screen::RuntimeSnapshot& snapshot = runtime.snapshot();
    const Screen32PageDescriptor* currentPage = runtime.currentPageDescriptor();
    const Screen32ElementDescriptor* element = screen32_find_element_descriptor(snapshot.lastEvent.elementId);

    std::printf("machine32 backend prep demo\n");
    std::printf("pages registered: %u\n", static_cast<unsigned>(screen32_page_descriptor_count()));
    std::printf("current page: %s (%u)\n",
                currentPage != nullptr ? currentPage->page_name : "<none>",
                static_cast<unsigned>(snapshot.currentPageId));
    std::printf("last event tag: %u, handled: %d\n",
                static_cast<unsigned>(snapshot.lastEvent.payloadTag),
                snapshot.lastEvent.handledByPage ? 1 : 0);
    std::printf("last element: %s (%u)\n",
                element != nullptr ? element->element_name : "<none>",
                static_cast<unsigned>(snapshot.lastEvent.elementId));
    std::printf("main menu section: %u\n", static_cast<unsigned>(mainMenuController.selectedSection()));

    return 0;
}
