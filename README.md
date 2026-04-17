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
- пример page controller: `src/Screen/main_menu_controller.*`

Его задача:
- зарегистрировать все generated pages из `ScreenUI/generated/shared`;
- держать `screenlib::PageRegistry` и `screenlib::ScreenSystem` на стороне backend;
- позволять подменять generated fallback для отдельных страниц своими `IPageController`;
- принимать и фиксировать входящие `Envelope`-события от `screen32`;
- позволять dry-run маршрутизации без concrete frontend adapter и без LVGL.

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
