#include "Screen/page/main/INIT.h"

#include <WiFi.h>
#include <esp_system.h>

#include "Catalog.h"
#include "Core.h"
#include "Machine/MachineSpec.h"
#include "Screen/page/main/info.h"
#include "Screen/page/main/input.h"
#include "Service/HServer.h"
#include "Service/WiFiConfig.h"
#include "version.h"

namespace {

// Локальное состояние экрана инициализации. Повторяет набор полей старого pINIT.
struct InitState {
    String group = "DEV";
    String name = "ESP32";
    String machineName;
    String machineList;
    bool accessPoint = false;
    bool withTestData = false;
};

InitState& initState() {
    static InitState state;
    return state;
}

// Возвращает первую поддерживаемую машину из каталога.
Catalog::MachineType firstSupportedMachine() {
    size_t count = 0;
    const Catalog::MachineType* types = Catalog::machineTypes(count);

    for (size_t i = 0; i < count; ++i) {
        const MachineSpec spec = MachineSpec::get(types[i]);
        if (spec.type() != Catalog::MachineType::UNKNOWN) {
            return types[i];
        }
    }

    return Catalog::MachineType::UNKNOWN;
}

String defaultMachineName() {
    const Catalog::MachineType machine = firstSupportedMachine();
    return machine == Catalog::MachineType::UNKNOWN ? String("") : Catalog::machineName(machine);
}

// Собирает список поддерживаемых машин в формате, подходящем для dropdown/Input.
String supportedMachineList(const char* separator = "\r\n") {
    size_t count = 0;
    const Catalog::MachineType* types = Catalog::machineTypes(count);
    String list;
    bool hasAny = false;

    for (size_t i = 0; i < count; ++i) {
        const MachineSpec spec = MachineSpec::get(types[i]);
        if (spec.type() == Catalog::MachineType::UNKNOWN) {
            continue;
        }

        if (hasAny && separator != nullptr) {
            list += separator;
        }

        list += Catalog::machineName(types[i]);
        hasAny = true;
    }

    return list;
}

bool tryResolveMachine(const String& rawName, Catalog::MachineType& outMachine) {
    size_t count = 0;
    const Catalog::MachineType* types = Catalog::machineTypes(count);

    for (size_t i = 0; i < count; ++i) {
        const MachineSpec spec = MachineSpec::get(types[i]);
        if (spec.type() == Catalog::MachineType::UNKNOWN) {
            continue;
        }

        if (rawName.equalsIgnoreCase(Catalog::machineName(types[i]))) {
            outMachine = types[i];
            return true;
        }
    }

    return false;
}

// Преобразует индекс выбранного пункта dropdown в тип машины.
Catalog::MachineType machineBySupportedIndex(int32_t index) {
    size_t count = 0;
    const Catalog::MachineType* types = Catalog::machineTypes(count);
    Catalog::MachineType firstSupported = Catalog::MachineType::UNKNOWN;
    int32_t currentIndex = 0;

    for (size_t i = 0; i < count; ++i) {
        const MachineSpec spec = MachineSpec::get(types[i]);
        if (spec.type() == Catalog::MachineType::UNKNOWN) {
            continue;
        }

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

// Возвращает индекс машины среди только поддерживаемых элементов каталога.
int32_t supportedMachineIndex(const String& machineName) {
    size_t count = 0;
    const Catalog::MachineType* types = Catalog::machineTypes(count);
    int32_t currentIndex = 0;

    for (size_t i = 0; i < count; ++i) {
        const MachineSpec spec = MachineSpec::get(types[i]);
        if (spec.type() == Catalog::MachineType::UNKNOWN) {
            continue;
        }

        if (machineName.equalsIgnoreCase(Catalog::machineName(types[i]))) {
            return currentIndex;
        }

        ++currentIndex;
    }

    return 0;
}

// Заполняет поля экрана значениями по умолчанию, если они ещё не заданы.
void ensureInitDefaults() {
    InitState& state = initState();

    if (state.group.isEmpty()) {
        state.group = "DEV";
    }
    if (state.name.isEmpty()) {
        state.name = "ESP32";
    }
    if (state.machineName.isEmpty()) {
        state.machineName = defaultMachineName();
    }
}

bool buildConfig(Catalog::MachineType machine,
                 const String& group,
                 const String& name,
                 bool accessPoint,
                 bool withTestData) {
    // Собирает config/data так же, как это делал старый pINIT перед перезапуском.
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

void returnToInit() {
    machine32::screen::Init::show();
}

// Показывает ошибку подключения WiFi и затем перезапускает устройство.
void showWiFiConnectErrorAndRestart(const char* errorText) {
    machine32::screen::Info::showInfo(
        "WiFi",
        "Ошибка подключения",
        errorText == nullptr ? "" : errorText,
        []() { esp_restart(); }
    );
}

void showHttpAddress(const String& title, const IPAddress& ip) {
    machine32::screen::Info::showInfo(
        title,
        "IP: " + ip.toString(),
        "",
        []() { machine32::screen::Init::show(); }
    );
}

void startHttpServerForConnectedWiFi(const IPAddress& ip) {
    HServer::getInstance().begin();
    showHttpAddress("HTTP сервер", ip);
}

void askManualWiFiPassword(const String& ssid);

// Запрашивает ручной ввод SSID, если сети по умолчанию нет или пользователь отказался.
void askManualWiFiCredentials() {
    machine32::screen::Input::showInput(
        "WiFi",
        "",
        "Введите название сети (SSID):",
        "",
        [](const String& ssidValue) {
            String ssid = ssidValue;
            ssid.trim();
            if (ssid.isEmpty()) {
                returnToInit();
                return;
            }

            askManualWiFiPassword(ssid);
        },
        []() { returnToInit(); },
        1,
        false
    );
}

void askManualWiFiPassword(const String& ssid) {
    machine32::screen::Input::showInput(
        "WiFi",
        "",
        "Введите пароль сети:",
        "",
        [ssid](const String& pass) {
            WiFiConfig& wifi = WiFiConfig::getInstance();
            const bool ok = wifi.connectWithCreds(ssid.c_str(), pass.c_str(), false);
            if (!ok) {
                showWiFiConnectErrorAndRestart(wifi.getLastError());
                return;
            }

            startHttpServerForConnectedWiFi(wifi.getIP());
        },
        []() { returnToInit(); },
        1,
        false
    );
}

void connectDefaultWiFiOrAskManual() {
    WiFiConfig& wifi = WiFiConfig::getInstance();
    const int defaultIndex = wifi.getDefaultIndex();
    const uint8_t count = wifi.getCount();

    if (defaultIndex < 0 || defaultIndex >= count) {
        askManualWiFiCredentials();
        return;
    }

    const String defaultSsid = wifi.getSSIDByIndex(static_cast<uint8_t>(defaultIndex));
    if (defaultSsid.isEmpty()) {
        askManualWiFiCredentials();
        return;
    }

    machine32::screen::Info::showInfo(
        "WiFi",
        "Подключиться к сети по умолчанию?",
        defaultSsid,
        [defaultSsid]() {
            WiFiConfig& wifi = WiFiConfig::getInstance();
            const bool ok = wifi.connectTo(defaultSsid.c_str());
            if (!ok) {
                showWiFiConnectErrorAndRestart(wifi.getLastError());
                return;
            }

            startHttpServerForConnectedWiFi(wifi.getIP());
        },
        []() { askManualWiFiCredentials(); },
        true
    );
}

// Поднимает точку доступа и HTTP сервер, как это делала ветка AP в pINIT.
void startAccessPointAndHttp() {
    WiFiConfig& wifi = WiFiConfig::getInstance();
    if (!WiFi.softAP("SMIT_" + String(APP_VERSION) + "_" + wifi.mac_xx())) {
        machine32::screen::Info::showInfo(
            "WiFi",
            "Ошибка запуска точки доступа",
            "",
            []() { machine32::screen::Init::show(); }
        );
        return;
    }

    HServer::getInstance().begin();
    showHttpAddress("HTTP сервер", WiFi.softAPIP());
}

// Сохраняет выбранные параметры инициализации и перезапускает устройство.
void saveInitConfig() {
    ensureInitDefaults();

    InitState& state = initState();
    Catalog::MachineType machine = Catalog::MachineType::UNKNOWN;
    if (!tryResolveMachine(state.machineName, machine)) {
        machine32::screen::Info::showInfo(
            "Ошибка",
            "Неизвестная машина",
            supportedMachineList(),
            []() { machine32::screen::Init::show(); }
        );
        return;
    }

    if (!buildConfig(machine, state.group, state.name, state.accessPoint, state.withTestData)) {
        machine32::screen::Info::showInfo(
            "Ошибка",
            "Не удалось сохранить",
            "config.json",
            []() { machine32::screen::Init::show(); }
        );
        return;
    }

    esp_restart();
}

}  // namespace

namespace machine32::screen {

void Init::onShow() {
    ensureInitDefaults();
    InitState& state = initState();

    pnl_INIT_TITLE.text = "Инициализация";
    btn_INIT_HTTP.text = "HTTP";
    btn_INIT_OK.text = "OK";
    state.machineList = supportedMachineList("\n");
    drp_INIT_MACHINE.text = state.machineList.c_str();
    drp_INIT_MACHINE.value = supportedMachineIndex(state.machineName);
    btn_INIT_GROUP.text = state.group.c_str();
    btn_INIT_NAME.text = state.name.c_str();
    chk_INIT_R_ACCESS_POINT.value = state.accessPoint ? 1 : 0;
    chk_INIT_R_TEST.value = state.withTestData ? 1 : 0;

    btn_INIT_HTTP.onClick = [this] { handleHttp(); };
    btn_INIT_OK.onClick = [this] { handleOk(); };
    btn_INIT_GROUP.onClick = [this] { handleGroup(); };
    btn_INIT_NAME.onClick = [this] { handleName(); };
    btn_INIT_ACCESS_POINT.onClick = [this] { handleAccessPoint(); };
    btn_INIT_TEST.onClick = [this] { handleTest(); };
}

void Init::handleHttp() {
    Info::showInfo(
        "WiFi",
        "Вы хотите подключиться к WiFi сети?",
        "",
        []() { connectDefaultWiFiOrAskManual(); },
        []() { startAccessPointAndHttp(); },
        true
    );
}

void Init::handleOk() {
    // Проверяем обязательные поля перед сохранением config/data.
    InitState& state = initState();
    state.group.trim();
    state.name.trim();

    if (state.group.isEmpty()) {
        Info::showInfo(
            "Ошибка",
            "Заполните поле",
            "Группа",
            []() { Init::show(); }
        );
        return;
    }

    if (state.name.isEmpty()) {
        Info::showInfo(
            "Ошибка",
            "Заполните поле",
            "Имя",
            []() { Init::show(); }
        );
        return;
    }

    saveInitConfig();
}

void Init::handleGroup() {
    // Редактирование группы вынесено в универсальный экран Input.
    Input::showInput(
        "Инициализация",
        "",
        "Введите группу:",
        initState().group,
        [](const String& groupValue) {
            String group = groupValue;
            group.trim();
            initState().group = group;
            Init::show();
        },
        []() { Init::show(); },
        1,
        false
    );
}

void Init::handleName() {
    // Редактирование имени вынесено в универсальный экран Input.
    Input::showInput(
        "Инициализация",
        "",
        "Введите имя:",
        initState().name,
        [](const String& nameValue) {
            String name = nameValue;
            name.trim();
            initState().name = name;
            Init::show();
        },
        []() { Init::show(); },
        1,
        false
    );
}

void Init::handleAccessPoint() {
    initState().accessPoint = !initState().accessPoint;
    Init::show();
}

void Init::handleTest() {
    initState().withTestData = !initState().withTestData;
    Init::show();
}

void Init::handleMachineChange(int32_t value) {
    // Dropdown присылает индекс, по нему обновляем выбранную машину в локальном state.
    const Catalog::MachineType machine = machineBySupportedIndex(value);
    if (machine != Catalog::MachineType::UNKNOWN) {
        initState().machineName = Catalog::machineName(machine);
    }
}

void Init::handleAccessPointChange(int32_t value) {
    initState().accessPoint = value != 0;
}

void Init::handleTestChange(int32_t value) {
    initState().withTestData = value != 0;
}

void Init::onInputInt(uint32_t elementId, int32_t value) {
    if (elementId == drp_INIT_MACHINE.id()) {
        handleMachineChange(value);
        return;
    }
    if (elementId == chk_INIT_R_ACCESS_POINT.id()) {
        handleAccessPointChange(value);
        return;
    }
    if (elementId == chk_INIT_R_TEST.id()) {
        handleTestChange(value);
        return;
    }
}

}  // namespace machine32::screen
