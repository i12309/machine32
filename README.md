# machine32

`machine32` — backend/host business project.

Он использует:
- `screenLIB` как host/core/client runtime и протокольный слой;
- `ScreenUI/generated/shared` как источник generated page/element ids и descriptors.

Он не должен зависеть от:
- LVGL;
- `ScreenUI/eez_project/src/ui`;
- `ScreenUI/generated/frontend_meta`;
- concrete frontend adapter (`EezLvglAdapter`, `UiObjectMap`).

## Локальная схема зависимостей

Рекомендуемое расположение рядом в workspace:
- `../screenLIB`
- `../ScreenUI`

Из UI-пакета `machine32` должен подключать только `../ScreenUI/generated/shared`.

## Направление развития

1. Реализовывать host/business-логику поверх `screenlib::ScreenSystem` и page controllers.
2. Использовать shared generated ids/descriptors из `ScreenUI/generated/shared` для маршрутизации и валидации.
3. Не переносить детали UI runtime в backend-код.

## Текущая подготовка под `screen32`

В репозитории добавлен backend-only runtime-слой:
- `src/Screen/runtime.*`
- `src/Screen/page.h`
- `src/Screen/page_factory.*`
- `src/Screen/Pages/main_menu_page.*`

Его задача:
- держать только одну активную backend-страницу в памяти;
- создавать page-object динамически по `page_id` через автогенерируемую фабрику;
- использовать `generic_page` как fallback для страниц, у которых бизнес-логика ещё не написана;
- скрывать детали `screenLIB` за простым фасадом `runtime`;
- принимать и фиксировать входящие `Envelope`-события от `screen32`;
- позволять dry-run маршрутизации без concrete frontend adapter и без LVGL.

## Новая модель описания страниц

Целевой сценарий для `machine32` теперь такой:

1. В `ScreenUI` появляется новая страница и её `page_id/element_id`.
2. `machine32` на сборке автоматически получает обновлённую generated factory.
3. Если для страницы ещё нет backend-класса, используется `generic_page`.
4. Когда нужна бизнес-логика, создаётся файл `src/Screen/Pages/<object_name>_page.h/.cpp`.
5. Класс страницы описывает только:
   - свой `page_id`;
   - `on_enter(...)`;
   - `on_button(...)`;
   - при необходимости `on_input_int(...)` и `on_input_text(...)`.

Ручная регистрация страниц больше не нужна.

Пока это именно слой подготовки, а не полное transport wiring старой прошивки:
- основной `src/main.cpp` не переведён на `screenLIB`;
- старый Nextion/UI контур не вырезан;
- runtime можно проверять отдельно через `backend_main.cpp`.

## Сборка подготовки

Для отдельной проверки backend prep есть env:

```bash
pio run -e screenprep_native
```

Для будущей ESP32-интеграции `screenLIB` и shared meta уже подключены в основном `env:esp32dev`.
