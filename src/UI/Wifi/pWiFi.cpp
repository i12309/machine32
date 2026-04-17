#include "pWiFi.h"
#include "UI/Main/pMain.h"
#include "UI/Main/pWAIT.h"
#include "UI/Main/pINPUT.h"
#include "UI/Main/pINFO.h"
#include "Catalog.h"

#include "Service/HServer.h"
#include "NexHardware.h"

namespace {

static const Page::TextPicToggleStyle kAutoConnectStyle = {
    Catalog::Color::yellow, // on bco
    Catalog::Color::lighter, // off bco
    Catalog::Color::white, // on pco (white)
    Catalog::Color::black, // off pco (dark)
    30,    // on pic
    29     // off pic
};

// Отправляет команду в Nextion и ждет подтверждения завершения.
bool sendCommandFinished(const String& cmd) {
    sendCommand(cmd.c_str());
    return recvRetCommandFinished();
}

// Очищает список ComboBox.
bool comboClear(NexObject& cb) {
    String cmd = String(cb.getObjName()) + ".clr()";
    return sendCommandFinished(cmd);
}

// Записывает список значений в ComboBox.
bool comboSetList(NexObject& cb, const String& list) {
    String cmd = String(cb.getObjName()) + ".path=\"" + list + "\"";
    return sendCommandFinished(cmd);
}

// Читает индекс выбранного пункта ComboBox.
bool comboGetIndex(NexObject& cb, uint32_t* index) {
    String cmd = "get " + String(cb.getObjName()) + ".val";
    sendCommand(cmd.c_str());
    return recvRetNumber(index);
}

// Устанавливает выбранный индекс ComboBox.
bool comboSetIndex(NexObject& cb, uint32_t index) {
    String cmd = String(cb.getObjName()) + ".val=" + String(index);
    return sendCommandFinished(cmd);
}

}

// Отображает страницу и синхронизирует UI с текущими данными.
void pWiFi::show() {
    Page::show();
    autoConnectDirty = false;
    refreshList();
    updateAutoConnect();
    updateConnectButton();
    updateConnectionInfo();
}

// Возврат в меню настроек.
void pWiFi::pop_bBack(void* ptr) {
    Log::D(__func__);
    pMain::getInstance().show();
}

// Сохраняет настройки выбранной сети и флаг автоподключения.
void pWiFi::pop_bSetWifi(void* ptr) {
    Log::D(__func__);
    pWiFi& UI = pWiFi::getInstance();
    WiFiConfig& wifi = WiFiConfig::getInstance();

    if (wifi.getCount() == 0) {
        Core::settings.CONNECT_WIFI = 0;
        Core::config.save();
        pMain::getInstance().show();
        return;
    }

    uint32_t index = UI.getSelectedIndex();
    if (index >= wifi.getCount()) return;

    const char* ssid = wifi.getSSIDByIndex(index);
    bool wantAuto = UI.autoConnectDirty
        ? UI.autoConnectPending
        : Page::isTextSelectedByColor(UI.tAutoConnect, kAutoConnectStyle.bcoOn);

    wifi.setDefaultIndex(static_cast<int>(index));
    wifi.save();

    Core::settings.CONNECT_WIFI = wantAuto ? 1 : 0;
    Core::config.save();
    UI.autoConnectDirty = false;

    pMain::getInstance().show();
}

// Подключает к выбранной сети или удаляет ее из списка.
void pWiFi::pop_bConnect(void* ptr) {
    Log::D(__func__);
    pWiFi& UI = pWiFi::getInstance();
    WiFiConfig& wifi = WiFiConfig::getInstance();

    if (wifi.getCount() == 0) return;

    String ssid = UI.getSelectedSSID();
    if (ssid.isEmpty()) return;

    if (!wifi.isConnectedTo(ssid.c_str())) {
        pWAIT::wait("","Соединение ... ","",1000,[ssid]() {
            WiFiConfig& wifi = WiFiConfig::getInstance();
            bool ok = wifi.connectTo(ssid.c_str());
            pWiFi::getInstance().show();
            if (ok) {
                if (Core::settings.HTTP_SERVER) HServer::getInstance().begin();
            } else {
                pINFO::showInfo("WiFi", "Ошибка подключения", wifi.getLastError());
            }
        }, false);
    } else {
        wifi.disconnect();
        pWiFi::getInstance().show();
    }
}

// Удаляет выбранную сеть без сохранения (с отключением при необходимости).
void pWiFi::pop_bDel(void* ptr) {
    Log::D(__func__);
    pWiFi& UI = pWiFi::getInstance();
    WiFiConfig& wifi = WiFiConfig::getInstance();

    if (wifi.getCount() == 0) return;

    String ssid = UI.getSelectedSSID();
    if (ssid.isEmpty()) return;

    pINFO::showInfo(
        "Удаление сети", "Подтвердите удаление", ssid,
        [ssid]() {
            WiFiConfig& wifi = WiFiConfig::getInstance();
            if (wifi.isConnectedTo(ssid.c_str())) {
                wifi.disconnect();
            }

            bool wasDefault = false;
            int defIndex = wifi.getDefaultIndex();
            if (defIndex >= 0) {
                const char* defSsid = wifi.getSSIDByIndex(defIndex);
                if (ssid == String(defSsid)) wasDefault = true;
            }

            wifi.remove(ssid.c_str());

            if (wasDefault && Core::settings.CONNECT_WIFI) {
                Core::settings.CONNECT_WIFI = 0;
                Core::config.save();
            }

            pWiFi::getInstance().show();
        },
        nullptr,
        true
    );
}

// Запускает добавление новой сети через форму ввода.
void pWiFi::pop_bAddItem(void* ptr) {
    Log::D(__func__);
    pINPUT::showInput(
        "Новая Wifi сеть", "", "Введите название сети (SSID):", "",
        [](const String& ssid) {
            if (ssid.isEmpty()) {
                pWiFi::getInstance().show();
                return;
            }
            String ssidCopy = ssid;
            pINPUT::showInput(
                "Новая Wifi сеть", "", "Введите пароль сети:", "",
                [ssidCopy](const String& pass) {
                    WiFiConfig& wifi = WiFiConfig::getInstance();
                    if (!wifi.addOrUpdate(ssidCopy.c_str(), pass.c_str())) {
                        pWiFi::getInstance().show();
                        pINFO::showInfo("WiFi", "Не удалось добавить сеть", wifi.getLastError());
                        return;
                    }
                    wifi.save();
                    pWiFi::getInstance().show();
                },
                [](){ pWiFi::getInstance().show(); },
                1,
                false
            );
        },
        [](){ pWiFi::getInstance().show(); },
        1,
        false
    );
}

// Периодически обновляет статус соединения и UI.
void pWiFi::n_Idle() {
    static unsigned long lastUpdateTime = 0;
    static const unsigned long updateInterval = 5000;

    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentTime;
        updateConnectionInfo();
        updateConnectButton();
        updateAutoConnect();
    }
}

// Переключает автоподключение по клику на текст.
void pWiFi::pop_tAutoConnect(void* ptr) {
    Log::D(__func__);
    pWiFi& UI = pWiFi::getInstance();
    bool selected = Page::isTextSelectedByColor(UI.tAutoConnect, kAutoConnectStyle.bcoOn);
    bool next = selected;
    if (selected == UI.autoConnectUiState) {
        next = !selected;
    }
    Page::setTextPicToggle(UI.tAutoConnect, UI.rAutoConnect, next, kAutoConnectStyle);
    UI.autoConnectUiState = next;
    UI.autoConnectPending = next;
    UI.autoConnectDirty = true;
}

// Заполняет ComboBox и скрывает/показывает элементы при пустом списке.
void pWiFi::refreshList() {
    WiFiConfig& wifi = WiFiConfig::getInstance();
    uint8_t count = wifi.getCount();

    if (count == 0) {
        comboClear(cbSSID);
        tRSSI.setText("");//Page::setVisible(tRSSI, false);
        tIP.setText("");//Page::setVisible(tIP, false);
        Page::setVisible(bConnect, false);
        Page::setVisible(bDel, false);
        return;
    }

    tRSSI.setText("Сигнал");//Page::setVisible(tRSSI, false);
    tIP.setText("IP-адрес");//Page::setVisible(tIP, false);
    Page::setVisible(bConnect, true);
    Page::setVisible(bDel, true);

    int defaultIndex = wifi.getDefaultIndex();
    String list;
    for (int i = 0; i < count; ++i) {
        if (i > 0) list += "\r\n";
        if (i == defaultIndex) list += "* ";
        list += wifi.getSSIDByIndex(i);
    }

    comboClear(cbSSID);
    comboSetList(cbSSID, list);

    int selectedIndex = wifi.getConnectedIndex();
    if (selectedIndex < 0) {
        if (defaultIndex >= 0 && defaultIndex < count) selectedIndex = defaultIndex;
        else selectedIndex = 0;
    }
    comboSetIndex(cbSSID, static_cast<uint32_t>(selectedIndex));
}

// Обновляет подпись и цвета кнопки подключения в зависимости от статуса.
void pWiFi::updateConnectButton() {
    WiFiConfig& wifi = WiFiConfig::getInstance();
    if (wifi.getCount() == 0) return;

    String ssid = getSelectedSSID();
    if (ssid.isEmpty()) return;

    bool connected = wifi.isConnectedTo(ssid.c_str());
    setConnectButtonState(connected);
}

// Обновляет поля RSSI и IP при наличии соединения.
void pWiFi::updateConnectionInfo() {
    WiFiConfig& wifi = WiFiConfig::getInstance();
    if (wifi.isConnect()) {
        int rssi = WiFi.RSSI();
        int rssiValue = (rssi == 0) ? 0 : 100 + rssi;
        String txt = "  "+String(rssiValue);
        tRSSIvalue.setText(txt.c_str());
        txt = "  "+wifi.getIP().toString();
        tIPvalue.setText(txt.c_str());
    } else {
        tRSSIvalue.setText("");
        tIPvalue.setText("");
    }
}

// Выставляет чекбокс автоподключения согласно текущему состоянию.
void pWiFi::updateAutoConnect() {
    WiFiConfig& wifi = WiFiConfig::getInstance();
    if (wifi.getCount() == 0) {
        Page::setTextPicToggle(tAutoConnect, rAutoConnect, false, kAutoConnectStyle);
        autoConnectUiState = false;
        autoConnectPending = false;
        autoConnectDirty = false;
        return;
    }

    if (autoConnectDirty) {
        Page::setTextPicToggle(tAutoConnect, rAutoConnect, autoConnectPending, kAutoConnectStyle);
        autoConnectUiState = autoConnectPending;
        return;
    }

    int value = Core::settings.CONNECT_WIFI ? 1 : 0;
    int defIndex = wifi.getDefaultIndex();
    if (value && (defIndex < 0 || defIndex >= wifi.getCount())) value = 0;
    Page::setTextPicToggle(tAutoConnect, rAutoConnect, value != 0, kAutoConnectStyle);
    autoConnectUiState = (value != 0);
}

// Возвращает индекс выбранной сети в ComboBox.
uint32_t pWiFi::getSelectedIndex() {
    uint32_t index = 0;
    comboGetIndex(cbSSID, &index);
    return index;
}

// Возвращает SSID выбранной сети.
String pWiFi::getSelectedSSID() {
    WiFiConfig& wifi = WiFiConfig::getInstance();
    uint32_t index = getSelectedIndex();
    if (index >= wifi.getCount()) return String("");
    return String(wifi.getSSIDByIndex(index));
}

// Меняет текст и цвета кнопки подключения по статусу соединения.
void pWiFi::setConnectButtonState(bool connected) {
    if (!connected) {
        bConnect.setText("Подключится");
        bConnect.Set_background_color_bco(Catalog::Color::green);
        bConnect.Set_press_background_color_bco2(Catalog::Color::yellow);
    } else {
        bConnect.setText("Отключиться");
        bConnect.Set_background_color_bco(Catalog::Color::orange);
        bConnect.Set_press_background_color_bco2(Catalog::Color::green);
    }
    bConnect.Set_font_color_pco(Catalog::Color::white);
}
