#include <cstdio>

#include "Screen/runtime.h"
#include "generated/shared/element_descriptors.generated.h"

int main() {
    screenlib::ScreenConfig config{};

    machine32::screen::runtime runtime;
    if (!runtime.init(config)) {
        std::printf("machine32 screen prep init failed: %s\n", runtime.last_error());
        return 1;
    }

    runtime.show_page(SCREEN32_PAGE_ID_MAIN_MENU);

    Envelope env = Envelope_init_zero;
    env.which_payload = Envelope_button_event_tag;
    env.payload.button_event.page_id = SCREEN32_PAGE_ID_MAIN_MENU;
    env.payload.button_event.element_id = SCREEN32_ELEMENT_ID_B_MAIN_TASK;
    runtime.dispatch_envelope_for_test(env);

    const machine32::screen::RuntimeSnapshot& snapshot = runtime.snapshot();
    const Screen32PageDescriptor* currentPage = runtime.current_page_descriptor();
    const Screen32ElementDescriptor* element = screen32_find_element_descriptor(snapshot.lastEvent.elementId);

    std::printf("machine32 backend prep demo\n");
    std::printf("pages in generated meta: %u\n", static_cast<unsigned>(screen32_page_descriptor_count()));
    std::printf("current page: %s (%u)\n",
                currentPage != nullptr ? currentPage->page_name : "<none>",
                static_cast<unsigned>(snapshot.currentPageId));
    std::printf("last event tag: %u, handled: %d\n",
                static_cast<unsigned>(snapshot.lastEvent.payloadTag),
                snapshot.lastEvent.handledByPage ? 1 : 0);
    std::printf("last element: %s (%u)\n",
                element != nullptr ? element->element_name : "<none>",
                static_cast<unsigned>(snapshot.lastEvent.elementId));

    return 0;
}
