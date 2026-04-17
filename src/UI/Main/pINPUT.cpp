#include "pINPUT.h"

pINPUT::InputCallback pINPUT::onOkHandler = nullptr;
T::THandlerFunction pINPUT::onCancelHandler = nullptr;
bool pINPUT::autoBackOnClose = true;

void pINPUT::showInput(const String& title, const String& info1, const String& info2, const String& defaultValue, InputCallback onOk, T::THandlerFunction onCancel, int showField, bool autoBack) {
    Log::D(__func__);
    pINPUT& page = getInstance();
    page.show();
    page.tTitle.setText(title.c_str());
    page.tInfo1.setText(info1.c_str());
    page.tInfo2.setText(info2.c_str());
    page.tInfo3.setText(defaultValue.c_str());
    page.showField.setValue(showField);

    if (!showField) {
        page.tInfo3.setText("");
        page.tField.setText("");
    } else page.tField.setText("   Введите\r\nинформацию");

    resetHandlers();
    onOkHandler = onOk;
    onCancelHandler = onCancel;
    autoBackOnClose = autoBack;
}

void pINPUT::pop_bOK(void* ptr){
    Log::D(__func__);
    pINPUT& page = getInstance();
    String value = page.getText(page.tInfo3, 32);
    value.trim();

    InputCallback callback = onOkHandler;
    bool shouldAutoBack = autoBackOnClose;
    resetHandlers();

    if (callback) callback(value);
    if (shouldAutoBack) page.back();
}

void pINPUT::pop_bCancel(void* ptr){
    Log::D(__func__);
    pINPUT& page = getInstance();
    T::THandlerFunction callback = onCancelHandler;
    bool shouldAutoBack = autoBackOnClose;
    resetHandlers();

    if (callback) callback();
    if (shouldAutoBack) page.back();
}

void pINPUT::resetHandlers(){
    onOkHandler = nullptr;
    onCancelHandler = nullptr;
    autoBackOnClose = true;
}
