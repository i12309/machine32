# machine32

`machine32` is the backend/host business project.

It consumes:
- `screenLIB` (host/core/client runtime and protocol)
- `ScreenUI/generated/shared` (generated page/element IDs and descriptors)

It must not depend on:
- LVGL
- `ScreenUI/eez_project/src/ui`
- `ScreenUI/generated/frontend_meta`
- concrete frontend adapter (`EezLvglAdapter`, `UiObjectMap`)

## Workspace Dependency Layout

Recommended local layout:
- `../screenLIB`
- `../ScreenUI`

`machine32` should include only `../ScreenUI/generated/shared` from UI package.

## Next Steps

1. Implement host/business logic over `screenlib::ScreenSystem` and page controllers.
2. Use shared generated IDs/descriptors from `ScreenUI/generated/shared` for routing/validation.
3. Keep UI runtime details out of backend code.
