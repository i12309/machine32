#pragma once

#include <stdint.h>

#include "generated/shared/element_descriptors.generated.h"
#include "generated/shared/page_descriptors.generated.h"
#include "pages/IPageController.h"
#include "pages/PageRegistry.h"
#include "system/ScreenSystem.h"

namespace machine32::screen {

struct CapturedScreenEvent {
    pb_size_t payloadTag = 0;
    uint32_t pageId = 0;
    uint32_t elementId = 0;
    bool handledByPage = false;
    bool hasIntValue = false;
    bool hasStringValue = false;
    int32_t intValue = 0;
    char stringValue[32] = {};
    screenlib::ScreenEventContext context{};
};

struct RuntimeSnapshot {
    uint32_t currentPageId = 0;
    CapturedScreenEvent lastEvent{};
};

class runtime {
public:
    bool addPageController(screenlib::IPageController* controller);
    bool init(const screenlib::ScreenConfig& config);
    void tick();

    bool selectPage(uint32_t pageId);
    bool dispatchEnvelopeForTest(const Envelope& env, const screenlib::ScreenEventContext& ctx = {});

    const RuntimeSnapshot& snapshot() const { return _snapshot; }
    const char* lastError() const { return _screens.lastError(); }

    screenlib::ScreenSystem& screens() { return _screens; }
    const screenlib::ScreenSystem& screens() const { return _screens; }

    const Screen32PageDescriptor* currentPageDescriptor() const;

private:
    class GeneratedPageController final : public screenlib::IPageController {
    public:
        void bind(const Screen32PageDescriptor* descriptor, runtime* owner);

        uint32_t pageId() const override;
        bool onEnvelope(const Envelope& env, const screenlib::ScreenEventContext& ctx) override;

    private:
        const Screen32PageDescriptor* _descriptor = nullptr;
        runtime* _owner = nullptr;
    };

    static void onScreenEvent(const Envelope& env, const screenlib::ScreenEventContext& ctx, void* userData);

    bool registerPages();
    bool hasCustomController(uint32_t pageId) const;
    void handleScreenEvent(const Envelope& env, const screenlib::ScreenEventContext& ctx);
    void capturePageEnvelope(uint32_t pageId,
                             const Envelope& env,
                             const screenlib::ScreenEventContext& ctx,
                             bool handledByPage);
    void captureEnvelopeCommon(const Envelope& env, const screenlib::ScreenEventContext& ctx, bool handledByPage);

    static constexpr size_t kMaxCustomPageControllers = screenlib::PageRegistry::kMaxPages;

    screenlib::ScreenSystem _screens;
    screenlib::PageRegistry _registry;
    GeneratedPageController _generatedControllers[SCREEN32_PAGE_DESCRIPTOR_COUNT] = {};
    screenlib::IPageController* _customControllers[kMaxCustomPageControllers] = {};
    size_t _customControllerCount = 0;
    RuntimeSnapshot _snapshot{};
    bool _initialized = false;
};

}  // namespace machine32::screen
