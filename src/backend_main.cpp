#include <cstdio>

#include "config/ScreenConfig.h"
#include "runtime/SinglePageRuntime.h"

#include "Screen/load_page.h"
#include "Screen/main_menu.h"

int main() {
    screenlib::ScreenConfig config{};

    screenlib::SinglePageRuntime rt;
    if (!rt.init(config)) {
        std::printf("backend init failed: %s\n", rt.lastError());
        return 1;
    }

    // Стартовая страница — никакой регистрации, просто тип.
    rt.start<machine32::screen::MainMenu>();

    std::printf("current page id: %u\n", static_cast<unsigned>(rt.currentPageId()));
    std::printf("connected physical: %d, web: %d\n",
                rt.connectedPhysical() ? 1 : 0,
                rt.connectedWeb() ? 1 : 0);

    rt.tick();
    return 0;
}
