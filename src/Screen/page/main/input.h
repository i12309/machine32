#pragma once

#include <cstring>
#include <functional>

#include "Screen/Screen.h"
#include "backend_pages/input_base.h"

namespace machine32::screen {

class Input : public screenui::InputPage<Input> {
public:
    using InputCallback = std::function<void(const String&)>;

    static bool show() {
        return Screen::getInstance().showPage<Input>();
    }

    static bool showInput(const String& title,
                          const String& info1,
                          const String& info2,
                          const String& defaultValue = "",
                          InputCallback onOk = nullptr,
                          std::function<void()> onCancel = nullptr,
                          int showField = 0,
                          bool autoBack = true) {
        DialogState& state = dialogState();
        state.title = title;
        state.info1 = info1;
        state.info2 = info2;
        state.value = defaultValue;
        state.onOk = onOk;
        state.onCancel = onCancel;
        state.showField = showField;
        state.autoBack = autoBack;
        return Input::show();
    }

protected:
    void onShow() override {
        const DialogState& state = dialogState();
        setElementText(pnl_INPUT_TITLE_1.id(), state.title.c_str());
        btn_INPUT_FIELD1.text = state.info1.c_str();
        btn_INPUT_FIELD2.text = state.info2.c_str();

        if (state.showField) {
            btn_INPUT_FIELD3.text = state.value.c_str();
            btn_INPUT_FIELD4.text = "   Введите\r\nинформацию";
            btn_INPUT_FIELD3.visible = true;
            btn_INPUT_FIELD4.visible = true;
        } else {
            btn_INPUT_FIELD3.text = "";
            btn_INPUT_FIELD4.text = "";
            btn_INPUT_FIELD3.visible = false;
            btn_INPUT_FIELD4.visible = false;
        }

        btn_INPUT_OK.onClick = [this] { handleOk(); };
        btn_INPUT_CANCEL.onClick = [this] { handleCancel(); };
    }

    void onInputText(uint32_t elementId, const char* text) override {
        if (elementId != btn_INPUT_FIELD3.id()) {
            return;
        }

        DialogState& state = dialogState();
        state.value = text == nullptr ? "" : text;
        btn_INPUT_FIELD3.text = state.value.c_str();
    }

private:
    struct DialogState {
        String title;
        String info1;
        String info2;
        String value;
        InputCallback onOk = nullptr;
        std::function<void()> onCancel = nullptr;
        int showField = 0;
        bool autoBack = true;
    };

    static DialogState& dialogState() {
        static DialogState state;
        return state;
    }

    static void resetState(DialogState& state) {
        state.onOk = nullptr;
        state.onCancel = nullptr;
        state.autoBack = true;
    }

    void setElementText(uint32_t elementId, const char* text) {
        if (runtime() == nullptr) {
            return;
        }

        const char* value = text == nullptr ? "" : text;
        runtime()->model().setString(elementId, ELEMENT_ATTRIBUTE_TEXT, value);

        ElementAttributeValue eav{};
        eav.attribute = ELEMENT_ATTRIBUTE_TEXT;
        eav.which_value = ElementAttributeValue_string_value_tag;
        std::strncpy(eav.value.string_value, value, sizeof(eav.value.string_value) - 1);
        eav.value.string_value[sizeof(eav.value.string_value) - 1] = '\0';
        runtime()->sendSetAttribute(elementId, eav);
    }

    void handleOk() {
        DialogState& state = dialogState();
        InputCallback callback = state.onOk;
        const String value = state.value;
        const bool shouldAutoBack = state.autoBack;
        resetState(state);

        if (callback) {
            callback(value);
        }
        if (shouldAutoBack) {
            Screen::getInstance().back();
        }
    }

    void handleCancel() {
        DialogState& state = dialogState();
        std::function<void()> callback = state.onCancel;
        const bool shouldAutoBack = state.autoBack;
        resetState(state);

        if (callback) {
            callback();
        }
        if (shouldAutoBack) {
            Screen::getInstance().back();
        }
    }
};

}  // namespace machine32::screen
