#pragma once

#include <functional>

#include "Screen/Screen.h"
#include "backend_pages/info_base.h"

namespace machine32::screen {

// Экран информационного диалога, повторяющий контракт старого pINFO.
class Info : public screenui::InfoBase<Info> {
public:
    static bool showInfo(String text1 = "", String text2 = "", String text3 = "",
                         std::function<void()> onOk = nullptr,
                         std::function<void()> onCancel = nullptr,
                         bool showCancel = false,
                         String okText = "",
                         String cancelText = "",
                         std::function<void()> onBack = nullptr,
                         std::function<void()> onNext = nullptr) {
        DialogState& state = dialogState();
        state.text1 = text1;
        state.text2 = text2;
        state.text3 = text3;
        state.onOk = onOk;
        state.onCancel = onCancel;
        state.onBack = onBack;
        state.onNext = onNext;
        state.showCancel = showCancel || (onCancel != nullptr);
        state.okText = okText;
        state.cancelText = cancelText;
        return Info::show();
    }

protected:
    void onShow() override {
        const DialogState& state = dialogState();

        //element(pnl_INFO_TITLE).setText("");
        element(btn_INFO_FIELD1).setText(state.text1.c_str());
        element(btn_INFO_FIELD2).setText(state.text2.c_str());
        element(btn_INFO_FIELD3).setText(state.text3.c_str());

        applyButtonTexts(state);

        element(btn_INFO_CANCEL).setVisible(state.showCancel);
        element(btn_INFO_BACK).setVisible(state.onBack != nullptr);
        element(btn_INFO_NEXT).setVisible(state.onNext != nullptr);

        if (state.onBack != nullptr) element(pnl_INFO_TITLE).setWidth(element(pnl_INFO_TITLE).getWidth() - element(btn_INFO_BACK).getWidth());
        if (state.onNext != nullptr) element(pnl_INFO_TITLE).setWidth(element(pnl_INFO_TITLE).getWidth() - element(btn_INFO_NEXT).getWidth());
    }

    void onClickInfoOk() override {
        DialogState& state = dialogState();
        std::function<void()> callback = state.onOk;
        resetCallbacks(state);
        if (callback) {
            callback();
            return;
        }

        Screen::getInstance().back();
    }

    void onClickInfoCancel() override {
        DialogState& state = dialogState();
        std::function<void()> callback = state.onCancel;
        resetCallbacks(state);
        if (callback) {
            callback();
            return;
        }

        Screen::getInstance().back();
    }

    void onClickInfoBack() override {
        DialogState& state = dialogState();
        std::function<void()> callback = state.onBack;
        resetCallbacks(state);
        if (callback) {
            callback();
            return;
        }

        Screen::getInstance().back();
    }

    void onClickInfoNext() override {
        DialogState& state = dialogState();
        std::function<void()> callback = state.onNext;
        resetCallbacks(state);
        if (callback) {
            callback();
        }
    }

private:
    static constexpr uint32_t kRequestInfoBackWidth = 0x494E4601u;
    static constexpr uint32_t kRequestInfoNextWidth = 0x494E4602u;
    static constexpr int32_t kDefaultNavButtonWidthPct = 15;

    struct DialogState {
        String text1;
        String text2;
        String text3;
        std::function<void()> onOk = nullptr;
        std::function<void()> onCancel = nullptr;
        std::function<void()> onBack = nullptr;
        std::function<void()> onNext = nullptr;
        bool showCancel = false;
        String okText;
        String cancelText;
    };

    static DialogState& dialogState() {
        static DialogState state;
        return state;
    }

    static void resetCallbacks(DialogState& state) {
        state.onOk = nullptr;
        state.onCancel = nullptr;
        state.onBack = nullptr;
        state.onNext = nullptr;
    }

    void applyButtonTexts(const DialogState& state) {
        const String okLabel = state.okText.length() > 0 ? state.okText : "OK";
        element(btn_INFO_OK).setText(okLabel.c_str());

        if (state.showCancel) {
            const String cancelLabel = state.cancelText.length() > 0 ? state.cancelText : "Отмена";
            element(btn_INFO_CANCEL).setText(cancelLabel.c_str());
        }
    }
};

}  // namespace machine32::screen
