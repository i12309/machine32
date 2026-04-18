#pragma once

#include <functional>

#include "Screen/Screen.h"
#include "backend_pages/input_base.h"

namespace machine32::screen {

// Экран ввода, повторяющий контракт старого pINPUT на новом runtime.
class Input : public screenui::InputBase {
public:
    // Тип обработчика подтверждения введенного значения.
    using InputCallback = std::function<void(const String&)>;

    // Показывает экран ввода и сохраняет обработчики кнопок.
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
        return Screen::getInstance().showPage<Input>();
    }

protected:
    // Заполняет экран сохраненным состоянием, как это делал старый pINPUT.
    void onShow() override {
        const DialogState& state = dialogState();
        element(txt_OBJ21).setText(state.title.c_str());
        element(btn_INPUT_FIELD1).setText(state.info1.c_str());
        element(btn_INPUT_FIELD2).setText(state.info2.c_str());

        if (state.showField) {
            element(btn_INPUT_FIELD3).setText(state.value.c_str());
            element(btn_INPUT_FIELD4).setText("   Введите\r\nинформацию");
            element(btn_INPUT_FIELD3).setVisible(true);
            element(btn_INPUT_FIELD4).setVisible(true);
        } else {
            element(btn_INPUT_FIELD3).setText("");
            element(btn_INPUT_FIELD4).setText("");
            element(btn_INPUT_FIELD3).setVisible(false);
            element(btn_INPUT_FIELD4).setVisible(false);
        }
    }

    // Подтверждает текущее значение и выполняет сохраненный обработчик.
    void onClickInputOk() override {
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

    // Отменяет ввод и выполняет сохраненный обработчик.
    void onClickInputCancel() override {
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

private:
    // Состояние диалога ввода между открытиями страницы.
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

    // Возвращает общее сохраненное состояние экрана ввода.
    static DialogState& dialogState() {
        static DialogState state;
        return state;
    }

    // Очищает обработчики после завершения диалога.
    static void resetState(DialogState& state) {
        state.onOk = nullptr;
        state.onCancel = nullptr;
        state.autoBack = true;
    }
};

}  // namespace machine32::screen
