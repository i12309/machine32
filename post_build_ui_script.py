import importlib.util
import hashlib
import json
import os
import re
import shutil
from pathlib import Path


def _load_nextion_parser(parser_path: Path):
    spec = importlib.util.spec_from_file_location("nextion2text_module", parser_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Failed to load parser module: {parser_path}")

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _extract_hmi_version(project_dir: Path) -> str:
    hmi_path = project_dir / "HMI" / "UI.HMI"
    parser_path = project_dir / "HMI" / "Nextion2Text" / "Nextion2Text.py"

    if not hmi_path.exists():
        raise FileNotFoundError(f"HMI file not found: {hmi_path}")
    if not parser_path.exists():
        raise FileNotFoundError(f"Parser file not found: {parser_path}")

    parser_module = _load_nextion_parser(parser_path)
    if not hasattr(parser_module, "HMI"):
        raise RuntimeError("Nextion parser does not expose HMI class")

    hmi = parser_module.HMI(hmi_path)
    fallback_component = None
    legacy_vtft_component = None

    for page in hmi.pages:
        page_id = getattr(page, "pageId", None)
        for component in page.components:
            attrs = component.rawData.get("att", {})
            if attrs.get("objname") == "verHMI":
                value = attrs.get("val")
                if value is None:
                    value = attrs.get("txt")
                if value is None:
                    raise ValueError("Component 'verHMI' found, but it has no 'val/txt' property")
                return str(value).strip()

            if attrs.get("objname") == "vTFT":
                legacy_vtft_component = attrs

            if page_id == 0 and attrs.get("id") == 17 and attrs.get("type") == 116:
                fallback_component = attrs

    if legacy_vtft_component is not None:
        value = legacy_vtft_component.get("txt")
        if value is not None:
            return str(value).strip()

    if fallback_component is not None:
        value = fallback_component.get("txt")
        if value is None:
            raise ValueError("Component page=0 id=17 found, but it has no 'txt' property")
        return str(value).strip()

    raise ValueError("Component 'verHMI' (or legacy 'vTFT') not found in HMI/UI.HMI")


def _write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def _write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")


def _calculate_md5(path: Path) -> str:
    md5 = hashlib.md5()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            md5.update(chunk)
    return md5.hexdigest()


def _read_esp_version(project_dir: Path) -> str:
    version_files = [
        project_dir / "src" / "version.h",
        project_dir / "src" / "config.h",
    ]
    version_pattern = re.compile(r'#define\s+APP_VERSION\s+"(\d+)"')
    for version_path in version_files:
        if not version_path.exists():
            continue
        for line in version_path.read_text(encoding="utf-8").splitlines():
            match = version_pattern.search(line)
            if match:
                return match.group(1)
    raise ValueError("APP_VERSION not found in src/version.h or src/config.h")


def _env_int(name: str, default: int) -> int:
    raw = os.getenv(name, "").strip()
    return int(raw) if raw.isdigit() else default


def _resolve_build_number(project_dir: Path) -> tuple[int, str]:
    explicit_build = os.getenv("BUILD_NUMBER", "").strip()
    explicit_source = os.getenv("BUILD_SOURCE", "").strip()
    if explicit_build.isdigit():
        return int(explicit_build), (explicit_source if explicit_source else "env(BUILD_NUMBER)")

    candidates = [
        ("CI_PIPELINE_IID", os.getenv("CI_PIPELINE_IID", "").strip()),
        ("GITHUB_RUN_NUMBER", os.getenv("GITHUB_RUN_NUMBER", "").strip()),
        ("CI_JOB_ID", os.getenv("CI_JOB_ID", "").strip()),
    ]
    for name, value in candidates:
        if value.isdigit():
            return int(value), f"env({name})"
    local_counter_file = project_dir / ".build_counter"
    if local_counter_file.exists():
        raw = local_counter_file.read_text(encoding="utf-8").strip()
        if raw.isdigit():
            return int(raw), f"local_counter({local_counter_file.name})"
    return 0, "fallback(0)"


def _release_dir_name(product_version: int, nextion_version: str, esp_version: str) -> str:
    hmi_part = nextion_version if str(nextion_version).isdigit() else "0"
    esp_part = esp_version if str(esp_version).isdigit() else "0"
    return f"{product_version}.{hmi_part}.{esp_part}"


def _copy_if_exists(src: Path, dst: Path) -> None:
    if not src.exists():
        return
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def post_build_ui(source, target, env):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    server_dir = project_dir / "server_files"
    output_file = server_dir / "ui.txt"

    nextion_version = "0"

    try:
        nextion_version = _extract_hmi_version(project_dir)
        _write_text(output_file, nextion_version)
        print(f"UI value written to: {output_file}")
    except Exception as error:
        error_message = f"ERROR: {error}"
        _write_text(output_file, error_message)
        print(f"Failed to extract UI value. Details written to: {output_file}")
        print(error_message)
        nextion_version = "0"

    try:
        esp_version = _read_esp_version(project_dir)
        product_version = _env_int("PRODUCT_VERSION", 1)
        build_number, build_source = _resolve_build_number(project_dir)

        firmware_file = server_dir / "firmware.bin"
        firmware_md5 = _calculate_md5(firmware_file) if firmware_file.exists() else ""
        if firmware_md5:
            _write_text(server_dir / "firmware.md5", firmware_md5)

        manifest = {
            "device_version": f"{product_version}.{nextion_version}.{esp_version}.{build_number}",
            "product": product_version,
            "nextion": int(nextion_version) if str(nextion_version).isdigit() else 0,
            "esp": int(esp_version),
            "build": build_number,
            "esp_file": "firmware.bin",
            "esp_md5": firmware_md5,
            "tft_file": "ui.tft",
            "tft_version_file": "ui.txt",
            "esp_version_file": "firmware.txt",
            "build_source": build_source,
        }

        manifest_path = server_dir / "device_manifest.json"
        _write_json(manifest_path, manifest)
        print(f"Device manifest written to: {manifest_path}")

        release_dir = server_dir / _release_dir_name(product_version, nextion_version, esp_version)
        release_dir.mkdir(parents=True, exist_ok=True)

        _copy_if_exists(server_dir / "firmware.bin", release_dir / "firmware.bin")
        _copy_if_exists(server_dir / "firmware.txt", release_dir / "firmware.txt")
        _copy_if_exists(server_dir / "firmware.md5", release_dir / "firmware.md5")
        _copy_if_exists(server_dir / "ui.tft", release_dir / "ui.tft")
        _copy_if_exists(server_dir / "ui.txt", release_dir / "ui.txt")
        _copy_if_exists(server_dir / "device_manifest.json", release_dir / "device_manifest.json")
        print(f"Release artifacts synced to: {release_dir}")
    except Exception as error:
        print(f"Failed to write device manifest: {error}")


Import("env")
env.AddPostAction("buildprog", post_build_ui)
