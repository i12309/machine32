#pragma once

#include <functional>

#include "Screen/Screen.h"
#include "backend_pages/info_base.h"

namespace machine32::screen {

class Info : public screenui::InfoPage<Info> {
public:
    static bool show() {
        return Screen::getInstance().showPage<Info>();
    }

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

        btn_INFO_FIELD1.text = state.text1.c_str();
        btn_INFO_FIELD2.text = state.text2.c_str();
        btn_INFO_FIELD3.text = state.text3.c_str();

        applyButtonTexts(state);

        btn_INFO_CANCEL.visible = state.showCancel;
        btn_INFO_BACK.visible = state.onBack != nullptr;
        btn_INFO_NEXT.visible = state.onNext != nullptr;

        if (state.onBack != nullptr) {
            pnl_INFO_TITLE.width = static_cast<int32_t>(pnl_INFO_TITLE.width) -
                                   static_cast<int32_t>(btn_INFO_BACK.width);
        }
        if (state.onNext != nullptr) {
            pnl_INFO_TITLE.width = static_cast<int32_t>(pnl_INFO_TITLE.width) -
                                   static_cast<int32_t>(btn_INFO_NEXT.width);
        }

        btn_INFO_OK.onClick = [this] { handleOk(); };
        btn_INFO_CANCEL.onClick = [this] { handleCancel(); };
        btn_INFO_BACK.onClick = [this] { handleBack(); };
        btn_INFO_NEXT.onClick = [this] { handleNext(); };
    }

private:
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
        btn_INFO_OK.text = okLabel.c_str();

        if (state.showCancel) {
            const String cancelLabel = state.cancelText.length() > 0 ? state.cancelText : "Отмена";
            btn_INFO_CANCEL.text = cancelLabel.c_str();
        }
    }

    void handleOk() {
        DialogState& state = dialogState();
        std::function<void()> callback = state.onOk;
        resetCallbacks(state);
        if (callback) {
            callback();
            return;
        }

        Screen::getInstance().back();
    }

    void handleCancel() {
        DialogState& state = dialogState();
        std::function<void()> callback = state.onCancel;
        resetCallbacks(state);
        if (callback) {
            callback();
            return;
        }

        Screen::getInstance().back();
    }

    void handleBack() {
        DialogState& state = dialogState();
        std::function<void()> callback = state.onBack;
        resetCallbacks(state);
        if (callback) {
            callback();
            return;
        }

        Screen::getInstance().back();
    }

    void handleNext() {
        DialogState& state = dialogState();
        std::function<void()> callback = state.onNext;
        resetCallbacks(state);
        if (callback) {
            callback();
        }
    }
};

}  // namespace machine32::screen
