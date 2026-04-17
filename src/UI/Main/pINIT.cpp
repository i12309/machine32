#include "pINIT.h"
#include "Catalog.h"
#include "UI/Main/pINFO.h"
#include "UI/Main/pINPUT.h"
#include "UI/Main/pWAIT.h"
#include "Core.h"
#include "Machine/MachineSpec.h"
#include "NexHardware.h"
#include "Service/HServer.h"
#include "Service/WiFiConfig.h"
#include <esp_system.h>
#include <WiFi.h>

namespace {

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

Catalog::MachineType machineByIndex(uint32_t index) {
    size_t count = 0;
    const Catalog::MachineType* types = Catalog::machineTypes(count);
    Catalog::MachineType firstSupported = Catalog::MachineType::UNKNOWN;
    uint32_t currentIndex = 0;

    for (size_t i = 0; i < count; ++i) {
        MachineSpec spec = MachineSpec::get(types[i]);
        if (spec.type() == Catalog::MachineType::UNKNOWN) continue;

        if (firstSupported == Catalog::MachineType::UNKNOWN) {
            firstSupported = types[i];
        }
        if (currentIndex == index) {
            return types[i];
        }
        ++currentIndex;
    }

    return firstSupported;
}

bool buildConfig(Catalog::MachineType machine, const String& group, const String& name, bool accessPoint, bool withTestData) {
    ConfigDefaults::Options options;
    options.machine = machine;
    options.group = group;
    options.name = name;
    options.accessPoint = accessPoint;
    options.withTestData = withTestData;

    if (!ConfigDefaults::build(Core::config.doc, options)) {
        return false;
    }

    Core::config.machine = Catalog::machineName(machine);
    Core::config.configVersion = options.configVersion;
    Core::config.name = name;
    Core::config.group = group;

    if (!Core::settings.load(Core::config.doc)) {
        return false;
    }

    if (!ConfigDefaults::buildData(Core::data.doc, withTestData)) {
        return false;
    }

    if (!Core::data.save()) {
        return false;
    }

    return Core::config.save();
}

}

void pINIT::prefill() {
    refreshMachineList();
    setSelectedMachineIndex(_lastMachineIndex);
    cAccessPoint.setValue(0);
    cTest.setValue(0);
    tGroup.setText("DEV");
    tName.setText("ESP32");
    _prefilled = true;
}

void pINIT::refreshMachineList() {
    size_t count = 0;
    const Catalog::MachineType* types = Catalog::machineTypes(count);
    String list;
    bool hasAny = false;

    for (size_t i = 0; i < count; ++i) {
        MachineSpec spec = MachineSpec::get(types[i]);
        if (spec.type() == Catalog::MachineType::UNKNOWN) continue;

        if (hasAny) list += "\r\n";
        list += Catalog::machineName(types[i]);
        hasAny = true;
    }

    comboClear(cbMachine);
    comboSetList(cbMachine, list);
}

uint32_t pINIT::getSelectedMachineIndex() {
    uint32_t index = 0;
    comboGetIndex(cbMachine, &index);
    return index;
}

void pINIT::setSelectedMachineIndex(uint32_t index) {
    comboSetIndex(cbMachine, index);
}

void pINIT::pop_bSave(void* ptr){
    Log::D(__func__);
    pINIT& UI = pINIT::getInstance();

    String group = UI.getText(UI.tGroup, 32);
    String name = UI.getText(UI.tName, 32);
    group.trim();
    name.trim();

    if (group.isEmpty()) {
        pINFO::showInfo("Ошибка", "Заполните поле", "Группа");
        return;
    }

    if (name.isEmpty()) {
        pINFO::showInfo("Ошибка", "Заполните поле", "Имя");
        return;
    }

    UI._lastMachineIndex = UI.getSelectedMachineIndex();
    Catalog::MachineType machine = machineByIndex(UI._lastMachineIndex);

    bool accessPoint = UI.getCBvalue(UI.cAccessPoint) == 1;
    bool withTestData = UI.getCBvalue(UI.cTest) == 1;

    if (!buildConfig(machine, group, name, accessPoint, withTestData)) {
        pINFO::showInfo("Ошибка", "Не удалось сохранить", "config.json");
        return;
    }

    esp_restart();
}

void showHttpIpWait(const String& title, const IPAddress& ip) {
    pWAIT& wait = pWAIT::getInstance();
    wait.show();
    wait.tText1.setText(title.c_str());
    String ipText = "IP: " + ip.toString();
    wait.tText2.setText(ipText.c_str());
    wait.tText3.setText("");
}

void showWiFiConnectErrorAndRestart(const char* errorText) {
    pINFO::showInfo(
        "WiFi",
        "Ошибка подключения",
        errorText == nullptr ? "" : errorText,
        []() { esp_restart(); }
    );
}

void startHttpServerForConnectedWiFi(const IPAddress& ip) {
    HServer::getInstance().begin();
    showHttpIpWait("HTTP сервер", ip);
}

void askManualWiFiCredentials() {
    pINPUT::showInput(
        "WiFi", "", "Введите название сети (SSID):", "",
        [](const String& ssid) {
            if (ssid.isEmpty()) {
                pINIT::getInstance().show();
                return;
            }
            String ssidCopy = ssid;
            pINPUT::showInput(
                "WiFi", "", "Введите пароль сети:", "",
                [ssidCopy](const String& pass) {
                    pWAIT::wait("", "Соединение ...", "", 1000, [ssidCopy, pass]() {
                        WiFiConfig& wifi = WiFiConfig::getInstance();

                        bool ok = wifi.connectWithCreds(ssidCopy.c_str(), pass.c_str(), false);
                        if (!ok) {
                            showWiFiConnectErrorAndRestart(wifi.getLastError());
                            return;
                        }

                        startHttpServerForConnectedWiFi(wifi.getIP());
                    }, false);
                },
                []() { pINIT::getInstance().show(); },
                1,
                false
            );
        },
        []() { pINIT::getInstance().show(); },
        1,
        false
    );
}

void connectDefaultWiFiOrAskManual() {
    WiFiConfig& wifi = WiFiConfig::getInstance();
    int defaultIndex = wifi.getDefaultIndex();
    uint8_t count = wifi.getCount();

    if (defaultIndex < 0 || defaultIndex >= count) {
        askManualWiFiCredentials();
        return;
    }

    String defaultSsid = wifi.getSSIDByIndex(static_cast<uint8_t>(defaultIndex));
    if (defaultSsid.isEmpty()) {
        askManualWiFiCredentials();
        return;
    }

    pINFO::showInfo(
        "WiFi",
        "Подключиться к сети по умолчанию?",
        defaultSsid,
        [defaultSsid]() {
            pWAIT::wait("", "Соединение ...", "", 1000, [defaultSsid]() {
                WiFiConfig& wifi = WiFiConfig::getInstance();
                bool ok = wifi.connectTo(defaultSsid.c_str());
                if (!ok) {
                    showWiFiConnectErrorAndRestart(wifi.getLastError());
                    return;
                }
                startHttpServerForConnectedWiFi(wifi.getIP());
            }, false);
        },
        []() { askManualWiFiCredentials(); },
        true
    );
}

void pINIT::pop_bHTTP(void* ptr) {
    Log::D(__func__);
    pINFO::showInfo(
        "WiFi",
        "Вы хотите подключиться к Wifi сети?",
        "",
        []() {
            connectDefaultWiFiOrAskManual();
        },
        []() {
            WiFiConfig& wifi = WiFiConfig::getInstance();
            if (!WiFi.softAP("SMIT_"+String(APP_VERSION)+"_"+wifi.mac_xx())) {
                pINFO::showInfo("WiFi", "Ошибка запуска точки доступа", "",
                    []() { pINIT::getInstance().show(); });
                return;
            }

            HServer::getInstance().begin();
            showHttpIpWait("HTTP сервер", WiFi.softAPIP());
        },
        true
    );
}
