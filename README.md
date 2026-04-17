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
