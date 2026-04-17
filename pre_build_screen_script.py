from __future__ import annotations

import re
from pathlib import Path


def _load_pages(page_descriptors_path: Path) -> list[dict[str, str | int]]:
    """Читает generated descriptors и извлекает список страниц screen32."""
    source = page_descriptors_path.read_text(encoding="utf-8")
    pattern = re.compile(r'\{(\d+)u,\s*"([A-Z0-9_]+)",\s*"([a-zA-Z0-9_]+)"\},')

    pages: list[dict[str, str | int]] = []
    for page_id_text, page_name, object_name in pattern.findall(source):
        pages.append(
            {
                "page_id": int(page_id_text),
                "page_name": page_name,
                "object_name": object_name,
            }
        )

    if not pages:
        raise RuntimeError(f"Не удалось извлечь список страниц из {page_descriptors_path}")

    return pages


def _macro_name(object_name: str) -> str:
    """Преобразует имя объекта страницы в безопасное имя макроса."""
    return re.sub(r"[^A-Za-z0-9]+", "_", object_name).upper()


def _render_factory_header(pages: list[dict[str, str | int]]) -> str:
    """Генерирует заголовок с фабрикой страниц для machine32.

    Смысл такой:
    - если для страницы существует пользовательский класс в `Screen/Pages/...`,
      фабрика создаёт именно его;
    - если класса ещё нет, создаётся `generic_page`, и backend всё равно может
      открыть страницу и работать с её элементами по ID.
    """
    lines = [
        "// АВТОСГЕНЕРИРОВАННЫЙ ФАЙЛ.",
        "// Источник: ScreenUI/generated/shared/page_descriptors.generated.h",
        "// НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ. Файл пересоздаётся pre-build скриптом machine32.",
        "",
        "#pragma once",
        "",
        "#include <memory>",
        "",
        "#include \"Screen/generic_page.h\"",
        "#include \"Screen/page.h\"",
        "#include \"generated/shared/page_descriptors.generated.h\"",
        "",
    ]

    for page in pages:
        object_name = str(page["object_name"])
        class_name = f"screen_page_{object_name}"
        include_path = f'Screen/Pages/{object_name}_page.h'
        macro_name = _macro_name(object_name)
        page_name = str(page["page_name"])
        page_id = int(page["page_id"])

        lines.extend(
            [
                f"// Страница {page_name}.",
                f'// Если проект содержит файл "{include_path}", будет создан',
                "// специализированный page-класс.",
                "// Иначе runtime автоматически использует универсальную заглушку.",
                f'#if __has_include("{include_path}")',
                f'#include "{include_path}"',
                f"#define MACHINE32_SCREEN_CREATE_{macro_name}() \\",
                f"    std::unique_ptr<machine32::screen::page>(new machine32::screen::{class_name}())",
                "#else",
                f"#define MACHINE32_SCREEN_CREATE_{macro_name}() \\",
                f"    std::unique_ptr<machine32::screen::page>(new machine32::screen::generic_page(screen32_find_page_descriptor({page_id}u)))",
                "#endif",
                "",
            ]
        )

    lines.extend(
        [
            "namespace machine32::screen::generated {",
            "",
            "// Возвращает новый объект страницы по ID.",
            "// Таблица создаётся автоматически на основе generated meta ScreenUI.",
            "inline std::unique_ptr<page> create_page(uint32_t page_id) {",
            "    switch (page_id) {",
        ]
    )

    for page in pages:
        object_name = str(page["object_name"])
        macro_name = _macro_name(object_name)
        page_id = int(page["page_id"])
        lines.append(f"        case {page_id}u: return MACHINE32_SCREEN_CREATE_{macro_name}();")

    lines.extend(
        [
            "        default:",
            "            return nullptr;",
            "    }",
            "}",
            "",
            f"static constexpr size_t kGeneratedPageCount = {len(pages)}u;",
            "",
            "}  // namespace machine32::screen::generated",
            "",
        ]
    )

    for page in pages:
        object_name = str(page["object_name"])
        macro_name = _macro_name(object_name)
        lines.append(f"#undef MACHINE32_SCREEN_CREATE_{macro_name}")

    lines.append("")
    return "\n".join(lines)


def _write_if_changed(path: Path, content: str) -> None:
    """Перезаписывает файл только если содержимое действительно изменилось."""
    path.parent.mkdir(parents=True, exist_ok=True)

    if path.exists():
        current = path.read_text(encoding="utf-8")
        if current == content:
            return

    path.write_text(content, encoding="utf-8", newline="\n")


def _run(project_dir: Path, env=None) -> None:
    screenui_shared_dir = project_dir.parent / "ScreenUI" / "generated" / "shared"
    page_descriptors_path = screenui_shared_dir / "page_descriptors.generated.h"
    factory_header_path = project_dir / "src" / "Screen" / "generated" / "page_factory.generated.h"

    pages = _load_pages(page_descriptors_path)
    factory_header = _render_factory_header(pages)
    _write_if_changed(factory_header_path, factory_header)

    # PageRegistry остаётся частью screenLIB, но machine32 должен видеть реальный
    # размер UI-схемы, а не условное число 16. Значение берём из тех же generated
    # descriptors, чтобы источник истины был один.
    if env is not None:
        env.Append(CPPDEFINES=[("SCREENLIB_PAGE_REGISTRY_MAX_PAGES", len(pages))])

    print(f"[screen] generated page factory: {factory_header_path}")
    print(f"[screen] page count for build: {len(pages)}")


try:
    Import("env")
except NameError:
    _run(Path(__file__).resolve().parent)
else:
    _run(Path(env.subst("$PROJECT_DIR")), env)
