# Объектная модель страницы — план переделки

Документ самодостаточный. Читается с нуля в новой сессии и отвечает
на вопрос "что делать дальше". Не требует внешнего контекста.

---

## 0. Контекст для новой сессии

### Структура проектов

```
c:/Users/sign/CODE/32/
├── screenLIB/      — библиотека: транспорт (UART + WebSocket), модель страниц
├── ScreenUI/       — мост между EEZ Studio и всем остальным
│                     (генератор backend-базовых классов,
│                      LVGL-адаптер для экрана)
├── Screen32/       — прошивка экрана на ESP32 (использует screenLIB + ScreenUI)
└── machine32/      — прошивка бэка станка (использует screenLIB + ScreenUI)
```

### Принятые решения (не пересматривать)

1. **Модель "бэк-оптимист, фронт-пессимист + очередь с backpressure".**
   - Бэк: set — сразу в локальную модель + в очередь отправки, код летит дальше.
   - Фронт: применяет только если LVGL согласился, шлёт подтверждение
     `AttributeChanged`. Если не смог отправить — откат локального UI.
   - Очередь на бэке: если растёт выше порога (>64 или >2 сек без ACK) →
     `linkDown()`, бэк ждёт ресинка.
   - На смене страницы — дренируем очередь и ждём `PageSnapshot`.

2. **Транспорт не трогаем.** nanopb остаётся. `ScreenBridge`, `FrameCodec`,
   `ProtoCodec`, `UartLink`, `WebSocketServerLink`, `WebSocketClientLink`,
   `ScreenClient`, `ScreenConfig*` — живые, не переписываем.

3. **Схлопываем луковицу в screenLIB-host.**
   - Удаляем: `ScreenSystem` (349), `ScreenManager` (303), `ScreenEndpoint`
     (105), `SinglePageRuntime` (237), `IHostPage.h`.
   - Создаём: один `PageRuntime` + `PageModel` + новый `Element` с
     typed-property-полями.

4. **Порядок работ — по проектам, не по этапам.**
   Сначала screenLIB целиком → потом ScreenUI → потом Screen32 →
   потом machine32. В каждом проекте своя ветка `object-model`.

5. **Процесс:** Claude пишет → показывает diff → пользователь ревьюит →
   принимает или задаёт вопросы. Коммиты делает Claude по запросу после
   одобрения.

6. **LVGL уже хранит всю модель элементов.** Не дублируем — просто
   сериализуем при необходимости через `lv_obj_get_*`.

### Словарь

- **Атрибут** — одно свойство элемента (width/height/visible/text/...).
  Список зафиксирован в `ElementAttribute` enum в proto.
- **Элемент** — один LVGL-объект, имеющий `element_id` из генератора.
- **Страница** — группа элементов под одним `page_id`.
- **Snapshot** — полный слепок состояния страницы (все элементы ×
  все synced-атрибуты).
- **Synced attrs** — подмножество LVGL-атрибутов элемента, которые
  реально синхронизируются (настройка в yaml-конфиге генератора).
- **Дельта** — точечное сообщение об изменении одного атрибута
  (`AttributeChanged`).

---

## 1. Целевая модель

### На бэке (как пишется код страницы)

```cpp
class Info : public screenui::InfoPage<Info> {
    void onShow() override {
        btn_INFO_BACK.visible = (state.onBack != nullptr);
        btn_INFO_NEXT.visible = (state.onNext != nullptr);

        int32_t w = pnl_INFO_TITLE.width;
        if (state.onBack) w -= btn_INFO_BACK.width;
        if (state.onNext) w -= btn_INFO_NEXT.width;
        pnl_INFO_TITLE.width = w;

        btn_INFO_FIELD1.text = state.text1.c_str();
        btn_INFO_OK.onClick  = [this]{ handleOk(); };
    }
};
```

- `btn_INFO_BACK`, `pnl_INFO_TITLE` — **поля сгенерированного класса**
  `InfoPage<Info>`. Тип поля зависит от типа LVGL-элемента (Button,
  Panel, Text, Slider и т.д.).
- Геттер — чтение из локальной `PageModel`. Мгновенно.
- Сеттер — запись в `PageModel` + push в `_pending` + отправка через
  bridge.
- Клики — через лямбду на поле `.onClick` либо через override метода
  `onClickXxx()`.

### На фронте (экран)

- `ShowPage(id)` → переключил LVGL → собрал `PageSnapshot` → отправил бэку.
- `SetElementAttribute(elem, attr, value, req_id)` → `lv_obj_set_*` →
  читает фактическое значение → шлёт `AttributeChanged(..., reason=COMMAND_APPLIED,
  in_reply_to=req_id)`. Это ACK.
- LVGL-событие `SIZE_CHANGED`/`VALUE_CHANGED` с коалесцингом в конце
  tick'а → `AttributeChanged(..., reason=LAYOUT|USER_INPUT)`.
- Кнопка → `ButtonEvent` как сейчас.
- Если `bridge.send()` провалился → откат LVGL-изменения (вернуть
  предыдущее значение), флаг "unsynced", ждать пока бэк пришлёт
  `ShowPage` заново.

---

## 2. Протокол (правки proto)

### Файлы

- `screenLIB/lib/core/src/proto/machine.proto` — редактируем руками.
- `screenLIB/lib/core/src/proto/machine.options` — добавляем размеры.
- `machine.pb.{h,c}` — генерируются автоматически при `pio run`.
  **Руками не редактировать.**

### Добавить в machine.proto

```proto
// Полный снимок состояния страницы (экран → бэк).
message PageSnapshot {
  uint32 page_id = 1;
  repeated ElementSnapshot elements = 2;
}

message ElementSnapshot {
  uint32 element_id = 1;
  repeated ElementAttributeValue attributes = 2;
}

// Переиспользуемый value с oneof — кирпичик для PageSnapshot и AttributeChanged.
message ElementAttributeValue {
  ElementAttribute attribute = 1;
  oneof value {
    int32 int_value = 2;
    uint32 color_value = 3;
    ElementFont font_value = 4;
    bool bool_value = 5;
    string string_value = 6;
  }
}

// Точечная дельта (экран → бэк). ACK команды или спонтанное изменение.
message AttributeChanged {
  uint32 page_id = 1;
  uint32 element_id = 2;
  ElementAttributeValue value = 3;
  AttributeChangeReason reason = 4;
  uint32 in_reply_to_request = 5;  // 0 если спонтанно (layout/user input)
}

enum AttributeChangeReason {
  REASON_UNKNOWN = 0;
  REASON_COMMAND_APPLIED = 1;
  REASON_LAYOUT = 2;
  REASON_USER_INPUT = 3;
  REASON_ANIMATION = 4;
}
```

Envelope — добавить в oneof payload:
```proto
PageSnapshot     page_snapshot     = 23;
AttributeChanged attribute_changed = 24;
```

### Расширить ElementAttribute enum

```proto
ELEMENT_ATTRIBUTE_VISIBLE = 8;
ELEMENT_ATTRIBUTE_TEXT    = 9;
ELEMENT_ATTRIBUTE_VALUE   = 10;
ELEMENT_ATTRIBUTE_X       = 11;
ELEMENT_ATTRIBUTE_Y       = 12;
```

### Добавить в machine.options

```
ElementAttributeValue.string_value   max_size:32
ElementSnapshot.attributes           max_count:16
PageSnapshot.elements                max_count:32
AttributeChanged.value               (вложенные строки тоже под max_size:32)
```

### НЕ удалять (пока) — legacy

`SetText`, `SetVisible`, `SetValue`, `SetColor`, `SetBatch`,
`ElementAttributeState`, `RequestPageState`, `PageState`,
`PageElementState`, `RequestElementState`, `ElementState`.
Оставляем до финальной чистки (этап 4 внутри machine32).

### DoD этапа 2 (протокол)

- `pio run` в screenLIB собирается без ошибок.
- В `machine.pb.h` видны `PageSnapshot`, `AttributeChanged`,
  `ElementAttributeValue`, новые теги `Envelope_page_snapshot_tag`,
  `Envelope_attribute_changed_tag`.
- Существующие unit-тесты screenLIB зелёные (`pio test -e native`).

---

## 3. Работа по проекту screenLIB

Ветка: `object-model` в `c:/Users/sign/CODE/32/screenLIB`.

### 3.1. Задачи

1. Правки proto + options (раздел 2). Пересборка → убедиться что .pb.{h,c}
   обновлены.
2. Создать новые файлы:
   - `lib/host/src/pages/PageModel.h/.cpp`
   - `lib/host/src/pages/Element.h/.cpp`
   - `lib/host/src/pages/IHostPage.h` (переписать с нуля, **сохранив имя**
     чтобы внешний код мог постепенно мигрировать — но внутри всё новое).
   - `lib/host/src/runtime/PageRuntime.h/.cpp`
3. Удалить старые файлы:
   - `lib/host/src/system/ScreenSystem.h/.cpp`
   - `lib/host/src/manager/ScreenManager.h/.cpp`
   - `lib/host/src/manager/ScreenEndpoint.h/.cpp`
   - `lib/host/src/runtime/SinglePageRuntime.h/.cpp`
4. Обновить юнит-тесты (`test/`) — если тесты ссылаются на удалённые
   классы, переписать под новые.
5. Собрать и прогнать тесты.

### 3.2. `PageModel` — контракт

Файл: `lib/host/src/pages/PageModel.h`.

```cpp
namespace screenlib {

struct AttributeValue {
    enum class Type : uint8_t { None, Int, Bool, Color, Font, String };
    Type type = Type::None;
    int32_t i = 0;
    uint32_t u = 0;
    bool b = false;
    ElementFont font = ELEMENT_FONT_UNKNOWN;
    // Строка — из пула страницы (см. ниже). Здесь только указатель.
    const char* s = nullptr;
};

class PageModel {
public:
    // Подготовка модели под страницу: очистка + резерв под элементы.
    // Вызывается при navigateTo перед ожиданием snapshot'а.
    void beginPage(uint32_t pageId);

    // Применить полный snapshot.
    void applySnapshot(const PageSnapshot& snap);

    // Применить дельту от экрана (экран канонично прав).
    void applyRemoteChange(const AttributeChanged& msg);

    // Чтение. Возвращает default-value если атрибута нет.
    int32_t     getInt (uint32_t elem, ElementAttribute a) const;
    bool        getBool(uint32_t elem, ElementAttribute a) const;
    uint32_t    getColor(uint32_t elem, ElementAttribute a) const;
    ElementFont getFont(uint32_t elem, ElementAttribute a) const;
    const char* getString(uint32_t elem, ElementAttribute a) const;

    // Локальная запись (оптимистичная). Бэк сразу видит новое значение.
    void setInt(uint32_t elem, ElementAttribute a, int32_t v);
    void setBool(uint32_t elem, ElementAttribute a, bool v);
    void setColor(uint32_t elem, ElementAttribute a, uint32_t v);
    void setFont(uint32_t elem, ElementAttribute a, ElementFont v);
    void setString(uint32_t elem, ElementAttribute a, const char* v);

    uint32_t pageId() const { return _pageId; }
    bool isReady() const { return _ready; }   // после applySnapshot
    void markReady() { _ready = true; }
    void clear();

private:
    uint32_t _pageId = 0;
    bool _ready = false;

    struct Slot {
        uint32_t elementId;
        ElementAttribute attr;
        AttributeValue value;
    };
    // Хранилище: плоский массив. Размер подбирается эмпирически:
    // макс. страница × макс. synced attrs. Для старта: 256 слотов ~ 4КБ.
    static constexpr size_t kMaxSlots = 256;
    Slot _slots[kMaxSlots];
    size_t _slotCount = 0;

    // Пул строк: fixed-size буфер на страницу.
    static constexpr size_t kStringPoolSize = 2048;
    char _stringPool[kStringPoolSize];
    size_t _stringPoolUsed = 0;

    Slot* findSlot(uint32_t elem, ElementAttribute a);
    const Slot* findSlot(uint32_t elem, ElementAttribute a) const;
    Slot* insertSlot(uint32_t elem, ElementAttribute a);
    const char* internString(const char* s);
};

}  // namespace screenlib
```

**Почему плоский массив, а не hash-map:**
- ESP32, нет STL hash-map в embedded-friendly виде без heap.
- На странице 20-50 элементов × 5-10 атрибутов = 100-500 слотов.
  Линейный поиск на 500 — микросекунды, некритично.
- Плоский массив → нет аллокаций, предсказуемая память.

**Пул строк:**
- `setString` копирует в пул, возвращает указатель в слот.
- `beginPage` сбрасывает `_stringPoolUsed = 0`.
- При переполнении — `Log::warn` и обрезка. Не критично: 2 КБ на
  страницу хватит с запасом.

### 3.3. `Element` — контракт

Файл: `lib/host/src/pages/Element.h`.

```cpp
namespace screenlib {

class PageRuntime;  // fwd

// Типизированный property. Чтение — из модели. Запись — в модель + команда.
template <typename T, ElementAttribute A>
class Property {
public:
    Property(PageRuntime* rt, uint32_t elem) : _rt(rt), _elem(elem) {}

    operator T() const;                 // read from model
    Property& operator=(T value);       // write to model + send

private:
    PageRuntime* _rt;
    uint32_t _elem;
};

// Сигнал. Страница регистрирует лямбду, runtime вызывает при событии.
template <typename... Args>
class Signal {
public:
    template <typename F> void operator=(F&& f) { _fn = std::forward<F>(f); }
    explicit operator bool() const { return bool(_fn); }
    void emit(Args... args) const { if (_fn) _fn(args...); }
private:
    std::function<void(Args...)> _fn;
};

// Базовый элемент — общий набор. Типизированные наследники (Button,
// Panel, Text, Slider) добавляют свои property. Наследники генерируются
// из yaml-конфига в ScreenUI.
class ElementBase {
public:
    ElementBase(PageRuntime* rt, uint32_t id) : _rt(rt), _id(id) {}

    Property<bool,    ELEMENT_ATTRIBUTE_VISIBLE>          visible{_rt, _id};
    Property<int32_t, ELEMENT_ATTRIBUTE_POSITION_WIDTH>   width  {_rt, _id};
    Property<int32_t, ELEMENT_ATTRIBUTE_POSITION_HEIGHT>  height {_rt, _id};

    uint32_t id() const { return _id; }
protected:
    PageRuntime* _rt;
    uint32_t _id;
};

// Конкретные типы — в generated/backend_pages/element_types.generated.h
// Пример того, что будет сгенерировано:
class Button : public ElementBase {
public:
    using ElementBase::ElementBase;
    Property<const char*, ELEMENT_ATTRIBUTE_TEXT>       text      {_rt, _id};
    Property<uint32_t,    ELEMENT_ATTRIBUTE_BACKGROUND_COLOR> bgColor {_rt, _id};
    Property<uint32_t,    ELEMENT_ATTRIBUTE_TEXT_COLOR> textColor {_rt, _id};
    Signal<> onClick;
};

class Panel : public ElementBase {
public:
    using ElementBase::ElementBase;
    Property<int32_t,  ELEMENT_ATTRIBUTE_X> x{_rt, _id};
    Property<int32_t,  ELEMENT_ATTRIBUTE_Y> y{_rt, _id};
    Property<uint32_t, ELEMENT_ATTRIBUTE_BACKGROUND_COLOR> bgColor{_rt, _id};
};

class Text : public ElementBase {
public:
    using ElementBase::ElementBase;
    Property<const char*, ELEMENT_ATTRIBUTE_TEXT>       text     {_rt, _id};
    Property<uint32_t,    ELEMENT_ATTRIBUTE_TEXT_COLOR> textColor{_rt, _id};
    Property<ElementFont, ELEMENT_ATTRIBUTE_TEXT_FONT>  font     {_rt, _id};
};

}  // namespace screenlib
```

### 3.4. `PageRuntime` — контракт

Файл: `lib/host/src/runtime/PageRuntime.h`.

Заменяет `SinglePageRuntime + ScreenSystem + ScreenManager + ScreenEndpoint`.

```cpp
namespace screenlib {

class IPage;  // fwd — базовый класс страницы (см. ниже)

class PageRuntime {
public:
    using LinkListener = void(*)(bool up, void* user);
    using PageFactory  = std::unique_ptr<IPage>(*)();

    // --- Lifecycle ---
    bool init(const ScreenConfig& cfg);  // поднимает bridge(ы) по конфигу
    void tick();                         // прокачивает bridge + таймауты очереди

    template <class T> bool navigateTo();
    bool back();

    // --- State ---
    bool linkUp() const;                 // все требуемые bridge connected
    bool pageSynced() const;             // snapshot получен и очередь пуста
    uint32_t pendingCommands() const;

    void setLinkListener(LinkListener l, void* user);

    // --- Для Element/Property (internal, но public для template) ---
    PageModel& model() { return _model; }
    uint32_t sendSetAttribute(uint32_t elem, const ElementAttributeValue& v);

    // --- Для страницы ---
    IPage* currentPage() { return _current.get(); }
    uint32_t currentPageId() const;

private:
    // Вместо ScreenManager/Endpoint — просто два владельца bridge.
    std::unique_ptr<ScreenBridge> _physical;
    std::unique_ptr<ScreenBridge> _web;
    MirrorMode _mode = MirrorMode::Auto;

    PageModel _model;
    std::unique_ptr<IPage> _current;
    PageFactory _currentFactory = nullptr;
    PageFactory _previousFactory = nullptr;

    // Очередь ожидающих ACK
    struct Pending {
        uint32_t id;
        uint32_t elementId;
        ElementAttribute attr;
        uint32_t sentAtMs;
    };
    static constexpr size_t kMaxPending = 64;
    static constexpr uint32_t kLinkTimeoutMs = 2000;
    Pending _pending[kMaxPending];
    size_t _pendingCount = 0;
    uint32_t _nextRequestId = 1;

    bool _linkUp = true;
    LinkListener _linkListener = nullptr;
    void* _linkListenerUser = nullptr;

    // Internal
    bool send(const Envelope& env);  // маршрутизация по _mode
    void onEnvelope(const Envelope& env);     // входящее
    void checkPendingTimeouts(uint32_t nowMs);
    void setLinkUp(bool up);
    bool swapCurrent(std::unique_ptr<IPage> next, PageFactory f);
    void drainPendingForNavigation();  // ждёт opustoшения или таймаута
};

// Базовый интерфейс страницы.
class IPage {
    friend class PageRuntime;
public:
    virtual ~IPage() = default;
    virtual uint32_t pageId() const = 0;

protected:
    virtual void onShow() {}
    virtual void onClose() {}
    virtual void onTick() {}

    // Вызов из runtime при получении ButtonEvent, до/после apply snapshot'а.
    virtual void onButton(uint32_t elementId, ButtonAction action) { (void)elementId; (void)action; }
    virtual void onInputInt(uint32_t elementId, int32_t value) { (void)elementId; (void)value; }
    virtual void onInputText(uint32_t elementId, const char* text) { (void)elementId; (void)text; }

    PageRuntime* runtime() { return _runtime; }
private:
    PageRuntime* _runtime = nullptr;
};

}  // namespace screenlib
```

### 3.5. Маршрутизация по `_mode`

Бывшие 16 `ByMode`-методов сжимаются в один:

```cpp
bool PageRuntime::send(const Envelope& env) {
    bool any = false, ok = true;
    if ((_mode == MirrorMode::Physical || _mode == MirrorMode::Both) && _physical) {
        ok &= _physical->send(env); any = true;
    }
    if ((_mode == MirrorMode::Web || _mode == MirrorMode::Both) && _web) {
        ok &= _web->send(env); any = true;
    }
    if (_mode == MirrorMode::Auto) {
        // "первый доступный"
        if (_physical && _physical->connected()) { ok = _physical->send(env); any = true; }
        else if (_web && _web->connected())      { ok = _web->send(env);      any = true; }
    }
    return any && ok;
}
```

### 3.6. Очередь команд и timeout

```cpp
uint32_t PageRuntime::sendSetAttribute(uint32_t elem, const ElementAttributeValue& v) {
    uint32_t reqId = _nextRequestId++;
    if (_pendingCount >= kMaxPending) { setLinkUp(false); return 0; }

    SetElementAttribute cmd{};
    cmd.element_id = elem;
    cmd.attribute  = v.attribute;
    // ... скопировать value в oneof по типу ...

    Envelope env = ...;
    bool sent = send(env);
    if (!sent) { setLinkUp(false); return 0; }

    _pending[_pendingCount++] = { reqId, elem, v.attribute, millis() };
    return reqId;
}

void PageRuntime::checkPendingTimeouts(uint32_t now) {
    if (_pendingCount == 0) return;
    if (now - _pending[0].sentAtMs > kLinkTimeoutMs) {
        setLinkUp(false);
    }
}

void PageRuntime::onEnvelope(const Envelope& env) {
    switch (env.which_payload) {
    case Envelope_attribute_changed_tag: {
        const auto& msg = env.payload.attribute_changed;
        // Удалить из очереди если in_reply_to_request != 0
        if (msg.in_reply_to_request != 0) {
            // линейный поиск и удаление
            for (size_t i = 0; i < _pendingCount; ++i) {
                if (_pending[i].id == msg.in_reply_to_request) {
                    _pending[i] = _pending[--_pendingCount];
                    break;
                }
            }
        }
        _model.applyRemoteChange(msg);
        break;
    }
    case Envelope_page_snapshot_tag: {
        _model.applySnapshot(env.payload.page_snapshot);
        _model.markReady();
        if (_current && !_current->_shown) {
            _current->onShow();
            _current->_shown = true;
        }
        break;
    }
    case Envelope_button_event_tag: ...  // dispatch to _current->onButton
    // ...
    }
}
```

### 3.7. Навигация

```cpp
template <class T>
bool PageRuntime::navigateTo() {
    // 1. Дренаж очереди со старой страницы (ждём ACK'ов или таймаут).
    drainPendingForNavigation();

    // 2. Создаём новую страницу, onClose старой.
    auto next = std::make_unique<T>();
    next->_runtime = this;
    if (_current) _current->onClose();
    _previousFactory = _currentFactory;
    _current = std::move(next);
    _currentFactory = []() -> std::unique_ptr<IPage> {
        return std::make_unique<T>();
    };

    // 3. Чистим модель, шлём ShowPage, ждём PageSnapshot.
    _model.beginPage(T::kPageId);
    Envelope env = ...;  // ShowPage(T::kPageId)
    if (!send(env)) { setLinkUp(false); return false; }

    // 4. onShow() вызывается из onEnvelope при приходе PageSnapshot.
    //    До этого момента страница в состоянии "not shown yet".
    return true;
}
```

### 3.8. DoD проекта screenLIB

- Ветка `object-model`, все правки в ней.
- `pio run` собирает без ошибок для native и esp32dev.
- `pio test -e native` — все существующие тесты зелёные.
- Новые unit-тесты:
  - `test_page_model`: beginPage → applySnapshot → getInt/setInt работают.
  - `test_page_runtime_queue`: sendSetAttribute → pending++ → приход
    AttributeChanged → pending--.
  - `test_page_runtime_timeout`: sendSetAttribute без ответа → через
    kLinkTimeoutMs → linkUp() == false → listener вызван.
  - `test_page_runtime_nav`: navigateTo → отправлен ShowPage → приход
    PageSnapshot → onShow() вызван.

---

## 4. Работа по проекту ScreenUI

Ветка: `object-model` в `c:/Users/sign/CODE/32/ScreenUI`.

### 4.1. Задачи

1. Создать `tools/ui_meta_gen/synced_attrs.yaml` — конфиг атрибутов.
2. Переписать `tools/ui_meta_gen/generate_ui_meta.py`:
   - убрать генерацию 12 избыточных диспатч-таблиц `onButton*` в
     `backend_pages/*_base.h`;
   - добавить генерацию типизированного класса страницы с полями-
     элементами;
   - добавить генерацию `element_types.generated.h` (Button/Panel/Text/
     Slider/... — по используемым типам).
3. Дополнить `adapter/lvgl_eez/EezLvglAdapter.{h,cpp}`:
   - `PageSnapshot buildPageSnapshot(uint32_t pageId);`
   - `bool applyAttribute(uint32_t elem, const ElementAttributeValue& v,
                          ElementAttributeValue& appliedOut);`
   - `void installChangeListeners(uint32_t pageId, ChangeCallback cb);`
   - Вспомогательные `readAttribute(elem, attr) → AttributeValue` для
     всех поддерживаемых типов.

### 4.2. Конфиг synced_attrs.yaml

```yaml
defaults: [visible]

TYPE_BUTTON:        [visible, width, height, text, bg_color, text_color]
TYPE_PANEL:         [visible, width, height, x, y, bg_color]
TYPE_CONTAINER:     [visible, width, height, x, y, bg_color]
TYPE_TEXT:          [visible, width, text, text_color, text_font]
TYPE_TEXTAREA:      [visible, width, text]
TYPE_IMAGE:         [visible, width, height, x, y]
TYPE_SLIDER:        [visible, width, value]
TYPE_CHECKBOX:      [visible, value, text]
TYPE_BAR:           [visible, width, value]
TYPE_SWITCH:        [visible, value]
TYPE_DROPDOWN:      [visible, value, text]
TYPE_IMAGE_BUTTON:  [visible, width, height]
TYPE_LED:           [visible, bg_color]
# остальные типы — только [visible] по умолчанию
```

### 4.3. Что генератор должен выдавать

#### `generated/shared/element_descriptors.generated.h`

Как сейчас + добавить:
```c
typedef struct Screen32ElementDescriptor {
    uint32_t element_id;
    const char* element_name;
    uint32_t page_id;
    Screen32ElementType type;
    uint16_t synced_mask;     // ← новое: битовая маска ElementAttribute
} Screen32ElementDescriptor;
```

#### `generated/backend_pages/info_base.h` (новый формат)

```cpp
#pragma once
#include "pages/IHostPage.h"
#include "pages/Element.h"
#include "element_types.generated.h"
#include "../shared/element_ids.generated.h"
#include "../shared/page_ids.generated.h"

namespace screenui {

template <typename TPage>
class InfoPage : public screenlib::IPage {
public:
    static constexpr uint32_t kPageId = scr_INFO;

    InfoPage()
      : pnl_INFO_TITLE  (this, ::pnl_INFO_TITLE)
      , btn_INFO_BACK   (this, ::btn_INFO_BACK)
      , btn_INFO_NEXT   (this, ::btn_INFO_NEXT)
      , btn_INFO_OK     (this, ::btn_INFO_OK)
      , btn_INFO_CANCEL (this, ::btn_INFO_CANCEL)
      , btn_INFO_FIELD1 (this, ::btn_INFO_FIELD1)
      , btn_INFO_FIELD2 (this, ::btn_INFO_FIELD2)
      , btn_INFO_FIELD3 (this, ::btn_INFO_FIELD3)
    {}

    uint32_t pageId() const final { return kPageId; }

    static bool show() { /* PageRuntime::global()->navigateTo<TPage>(); */ }

protected:
    screenlib::Panel  pnl_INFO_TITLE;
    screenlib::Button btn_INFO_BACK;
    screenlib::Button btn_INFO_NEXT;
    screenlib::Button btn_INFO_OK;
    screenlib::Button btn_INFO_CANCEL;
    screenlib::Button btn_INFO_FIELD1;
    screenlib::Button btn_INFO_FIELD2;
    screenlib::Button btn_INFO_FIELD3;

private:
    void onButton(uint32_t elementId, ButtonAction action) final {
        if (action != ButtonAction_CLICK) return;
        switch (elementId) {
            case ::btn_INFO_BACK:   btn_INFO_BACK.onClick.emit();   break;
            case ::btn_INFO_NEXT:   btn_INFO_NEXT.onClick.emit();   break;
            case ::btn_INFO_OK:     btn_INFO_OK.onClick.emit();     break;
            case ::btn_INFO_CANCEL: btn_INFO_CANCEL.onClick.emit(); break;
            case ::btn_INFO_FIELD1: btn_INFO_FIELD1.onClick.emit(); break;
            case ::btn_INFO_FIELD2: btn_INFO_FIELD2.onClick.emit(); break;
            case ::btn_INFO_FIELD3: btn_INFO_FIELD3.onClick.emit(); break;
        }
    }
};

}  // namespace screenui
```

Сравнение:
- Было: 7 виртуальных методов `onButtonXxx` + 7 виртуальных
  `onClickXxx` + диспатч в `onButton()`. 50 строк на страницу.
- Стало: конструктор-инициализатор + 1 `onButton`-диспатч в сигналы.
  Страница бэка назначает лямбды через `btn_X.onClick = ...`.

#### `generated/backend_pages/element_types.generated.h` (новый файл)

Определения `Button`, `Panel`, `Text`, `Slider`, etc. — на основе
synced_attrs.yaml. Это то, что в плане в разделе 3.3 было показано
как пример.

### 4.4. EezLvglAdapter — дополнение

Новые функции:

```cpp
// ScreenUI/adapter/lvgl_eez/EezLvglAdapter.h

// Собрать snapshot текущей страницы (всё, что synced).
bool eez_build_page_snapshot(uint32_t page_id, PageSnapshot& out);

// Применить атрибут, вернуть фактическое значение.
bool eez_apply_attribute(uint32_t elem_id,
                         const ElementAttributeValue& in,
                         ElementAttributeValue& applied_out);

// Подписка на изменения. cb вызывается с коалесцингом в конце tick.
using AttributeChangeCb = void(*)(uint32_t elem_id,
                                  const ElementAttributeValue& val,
                                  AttributeChangeReason reason,
                                  void* user);
void eez_install_change_listeners(uint32_t page_id,
                                  AttributeChangeCb cb, void* user);

// Вызывать в конце фронт-tick для отправки коалесцированных дельт.
void eez_flush_pending_changes();
```

### 4.5. DoD проекта ScreenUI

- Ветка `object-model`.
- `python tools/ui_meta_gen/generate_ui_meta.py` работает без ошибок.
- Сгенерированные файлы компилируются (проверяется сборкой machine32 и
  Screen32 — но **сами не переписываем** пока, это этапы 5-6).
- Adapter собирается.

---

## 5. Работа по проекту Screen32

Ветка: `object-model` в `c:/Users/sign/CODE/32/Screen32`.

### 5.1. Задачи

1. Создать `src/common_app/FrontApp.{h,cpp}` — единый фасад.
2. Удалить/переписать:
   - `frontend_runtime.cpp` (692 строки) → функционал перенесён в FrontApp.
   - `frontend_ui_events.cpp` (217) → упрощается до 50 строк.
   - `page_state_builder.cpp` (306) → **удалить**, заменяется
     `eez_build_page_snapshot`.
   - `frontend_service_responder.cpp` (118) → 50 строк, остаются
     только `DeviceInfo` и переход в offline-demo.
3. Создать `src/demo/` шаблоны для пользовательских offline-страниц
   ("для тупых"):
   - `demo/README.md` — как подключить свою страницу.
   - `demo/example_page.cpp` — мини-пример.
4. Привести `main.cpp` и `web_main.cpp` к новому фасаду.

### 5.2. `FrontApp` — контракт

```cpp
namespace frontapp {

struct Config {
    bool allow_physical = true;
    bool allow_web      = true;
    // ... (берётся из frontend_config.json)
};

bool init(const Config& cfg);
void tick();

// Режимы
bool is_online();
bool is_offline_demo();
bool switch_to_offline_demo();

// Наблюдатели
bool backend_connected();
uint32_t current_page();

}  // namespace frontapp
```

Внутри `FrontApp.cpp`:
- Инициализирует `ScreenClient` (транспорт).
- Регистрирует callback на входящие Envelope.
- Обрабатывает команды:
  - `ShowPage` → переключить LVGL → `eez_build_page_snapshot` → отправить.
  - `SetElementAttribute` → `eez_apply_attribute` → отправить
    `AttributeChanged(COMMAND_APPLIED, in_reply_to=req_id)`.
  - `SetText/SetVisible/SetValue/...` (legacy) — продолжаем обрабатывать.
  - Service-запросы (`RequestDeviceInfo` и т.д.) — отвечаем как раньше.
- Ставит `eez_install_change_listeners` на активную страницу.
- В `tick` — дёргает `ScreenClient::tick()`, `eez_flush_pending_changes()`.
- Если offline_demo — вызывает пользовательские хуки из `src/demo/`.

### 5.3. Фолбек при обрыве связи

- `ScreenClient::isConnected()` → false → `FrontApp` не дёргает backend,
  но продолжает отрисовывать LVGL.
- При попытке LVGL-изменения не отправить дельту (connected == false) —
  **откатить** изменение в LVGL через ещё один `lv_obj_set_*` с
  предыдущим значением. Предыдущее значение хранится в "shadow" буфере,
  который заполняется при `eez_apply_attribute` и
  `eez_build_page_snapshot`.
- При восстановлении связи — ждём от бэка `ShowPage` (бэк
  ресинхронизируется после `linkUp`).

### 5.4. DoD проекта Screen32

- Ветка `object-model`.
- `pio run -e esp32dev` собирает.
- Прошивается на железо.
- Ручная проверка:
  - В offline-demo режиме: крутится демо-страница, клики работают.
  - В online режиме: подключается бэк, видна текущая страница, клики
    уходят на бэк.
  - Обрыв UART: фронт уходит в "нет связи", при восстановлении —
    ресинхронизируется.

---

## 6. Работа по проекту machine32

Ветка: `object-model` в `c:/Users/sign/CODE/32/machine32`.

### 6.1. Задачи

1. `src/Screen/Screen.{h,cpp}` — перенести с `SinglePageRuntime` на
   `PageRuntime`.
2. `src/Screen/page/main/load.h` — эталон на новом API.
3. `src/Screen/page/main/main.h` — эталон на новом API.
4. Остальные страницы (`info.h`, `input.h`, `INIT.cpp`) —
   **временно убрать из сборки**. Пользователь перепишет сам, вдохновляясь
   эталонами.
5. Финальная чистка — удалить legacy-сообщения из proto (вернуться в
   screenLIB, отдельный PR после того как machine32 собран).

### 6.2. Screen.cpp — верхний фасад

Упрощается. API остаётся прежним для вызывающего кода в App.cpp,
меняется только внутренность — вместо `_runtime` типа `SinglePageRuntime`
там `PageRuntime`. `showPage<T>()`, `back()`, `tick()` — те же сигнатуры.

### 6.3. Эталон load.h

```cpp
#pragma once
#include <WiFi.h>
#include "Screen/Screen.h"
#include "Screen/page/main/main.h"
#include "State/State.h"
#include "backend_pages/load_base.h"
#include "version.h"

namespace machine32::screen {

class Load : public screenui::LoadPage<Load> {
protected:
    void onShow() override {
        _openedMain = false;
        Screen& screen = Screen::getInstance();
        const String v = Version::makeDeviceVersion(screen.getScreenVersion());
        txt_LOAD_VERSION.text     = v.c_str();
        txt_LOAD_MA_CADDRESS.text = WiFi.macAddress().c_str();
    }

    void onTick() override {
        State* s = App::state();
        if (!s || _openedMain) return;
        if (s->type() == State::Type::IDLE) {
            _openedMain = true;
            Main::show();
        }
    }

private:
    bool _openedMain = false;
};

}  // namespace machine32::screen
```

### 6.4. Эталон main.h

```cpp
#pragma once
#include "Screen/Screen.h"
#include "backend_pages/main_base.h"

namespace machine32::screen {

class Main : public screenui::MainPage<Main> {
protected:
    void onShow() override {
        btn_MAIN_TASK.onClick    = []{ /* Task::show(); */ };
        btn_MAIN_PROFILE.onClick = []{ /* ProfileList::show(); */ };
        btn_MAIN_NET.onClick     = []{ /* Net::show(); */ };
        btn_MAIN_SERVICE.onClick = []{ /* Service::show(); */ };
        btn_MAIN_STATS.onClick   = []{ /* Stats::show(); */ };
        btn_MAIN_SUPPORT.onClick = []{ /* Help::show(); */ };
    }
};

}  // namespace machine32::screen
```

### 6.5. DoD проекта machine32

- Ветка `object-model`.
- `pio run -e esp32dev` собирает machine32 с уменьшенным набором страниц
  (Load + Main + то, что не удалось отключить).
- Прошивается.
- На железе: boot → Load с версией и MAC → переход на Main → клики
  логируются (даже если обработчики пустые).

### 6.6. Чистка legacy

После того как machine32 работает на новом API — возвращаемся в
screenLIB, удаляем:
- `SetText`, `SetVisible`, `SetValue`, `SetColor`, `SetBatch` из proto.
- `ElementAttributeState`, `RequestPageState`, `PageState`,
  `PageElementState`, `RequestElementState`, `ElementState` из proto.
- Соответствующие методы в ScreenBridge.
- Обработчики в Screen32/FrontApp.
- Перегенерация .pb.
- Финальная сборка всех 4 проектов.

---

## 7. Порядок работ и сроки

| Проект | Срок | DoD |
|---|---|---|
| 1. screenLIB | 3-4 дня | Сборка + unit-тесты зелёные |
| 2. ScreenUI | 1.5 дня | Генератор работает + adapter собирается |
| 3. Screen32 | 2 дня | Железо, online + offline работают |
| 4. machine32 | 1 день | Железо, Load → Main работает |
| 5. Чистка legacy (обратно в screenLIB) | 0.5 дня | Всё собирается без legacy proto |
| **Итого** | **~8 дней** | |

С учётом ревью и отладки на железе — **~2 недели календарно**.

---

## 8. Процесс работы в сессии

1. Claude работает в ветке `object-model` нужного проекта.
2. Пишет код, **не коммитит**.
3. Показывает diff пользователю: "готов правки X, посмотри".
4. Пользователь ревьюит, пишет "ок" / "поменяй то-то".
5. После "ок" — Claude коммитит в ветку.
6. После завершения проекта (его DoD) — Claude говорит "готов к мержу,
   запусти сборку / прошивку".
7. Пользователь собирает и мержит сам.

---

## 9. Что читать в новой сессии перед стартом

### Минимум для входа в курс

1. Этот файл (`machine32/object.md`) целиком.
2. `screenLIB/lib/core/src/proto/machine.proto` — чтобы видеть текущий
   протокол.
3. Текущий проект, в котором работаем (один из 4) — ветка `object-model`
   если уже есть, иначе `main`.

### Команды для ориентации

```bash
# В каком проекте и ветке мы
cd c:/Users/sign/CODE/32/screenLIB && git branch --show-current
cd c:/Users/sign/CODE/32/ScreenUI && git branch --show-current
cd c:/Users/sign/CODE/32/Screen32 && git branch --show-current
cd c:/Users/sign/CODE/32/machine32 && git branch --show-current

# Последние коммиты в каждом
git -C c:/Users/sign/CODE/32/screenLIB log --oneline -5
git -C c:/Users/sign/CODE/32/ScreenUI log --oneline -5
git -C c:/Users/sign/CODE/32/Screen32 log --oneline -5
git -C c:/Users/sign/CODE/32/machine32 log --oneline -5
```

### Текущий статус работ

**Старт.** План утверждён. Веток `object-model` ещё нет.
Следующий шаг: создать ветку `object-model` в `screenLIB`, начать с
правок `machine.proto` (раздел 2).

Когда начнётся следующая сессия — отметить здесь, на каком шаге
остановились:

- [ ] screenLIB/object-model: ветка создана
- [ ] screenLIB/object-model: proto отредактирован
- [ ] screenLIB/object-model: .pb.{h,c} перегенерированы
- [ ] screenLIB/object-model: PageModel написан
- [ ] screenLIB/object-model: Element написан
- [ ] screenLIB/object-model: PageRuntime написан
- [ ] screenLIB/object-model: старые файлы удалены
- [ ] screenLIB/object-model: unit-тесты зелёные
- [ ] screenLIB/object-model: влит в main
- [ ] ScreenUI/object-model: ветка создана
- [ ] ScreenUI/object-model: synced_attrs.yaml создан
- [ ] ScreenUI/object-model: генератор переписан
- [ ] ScreenUI/object-model: adapter дополнен
- [ ] ScreenUI/object-model: влит в main
- [ ] Screen32/object-model: ветка создана
- [ ] Screen32/object-model: FrontApp написан
- [ ] Screen32/object-model: старые frontend_*.cpp удалены
- [ ] Screen32/object-model: собрано и прошито
- [ ] Screen32/object-model: проверено online + offline
- [ ] Screen32/object-model: влит в main
- [ ] machine32/object-model: ветка создана
- [ ] machine32/object-model: Screen.cpp на PageRuntime
- [ ] machine32/object-model: Load эталон
- [ ] machine32/object-model: Main эталон
- [ ] machine32/object-model: собрано и прошито
- [ ] machine32/object-model: влит в main
- [ ] Финальная чистка legacy в proto
