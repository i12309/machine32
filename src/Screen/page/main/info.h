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
                         String cancelText = "") {
        DialogState& state = dialogState();
        state.text1 = text1;
        state.text2 = text2;
        state.text3 = text3;
        state.onOk = onOk;
        state.onCancel = onCancel;
        state.showCancel = showCancel || (onCancel != nullptr);
        state.okText = okText;
        state.cancelText = cancelText;
        return Info::show();
    }

protected:
    void onShow() override {
        const DialogState& state = dialogState();
        element(pnl_INFO_TITLE).setText("");
        element(btn_INFO_FIELD1).setText(state.text1.c_str());
        element(btn_INFO_FIELD2).setText(state.text2.c_str());
        element(btn_INFO_FIELD3).setText(state.text3.c_str());
        applyButtonTexts(state);
        applyCancelVisibility(state.showCancel);
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

private:
    struct DialogState {
        String text1;
        String text2;
        String text3;
        std::function<void()> onOk = nullptr;
        std::function<void()> onCancel = nullptr;
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
    }

    void applyButtonTexts(const DialogState& state) {
        const String okLabel = state.okText.length() > 0 ? state.okText : "OK";
        element(btn_INFO_OK).setText(okLabel.c_str());

        if (state.showCancel) {
            const String cancelLabel = state.cancelText.length() > 0 ? state.cancelText : "Отмена";
            element(btn_INFO_CANCEL).setText(cancelLabel.c_str());
        }
    }

    void applyCancelVisibility(bool visible) {
        element(btn_INFO_CANCEL).setVisible(visible);
    }
};

}  // namespace machine32::screen
