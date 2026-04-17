import os
from pathlib import Path


def _resolve_from_ci() -> tuple[int, str] | None:
    candidates = [
        ("BUILD_NUMBER", os.getenv("BUILD_NUMBER", "").strip()),
        ("CI_PIPELINE_IID", os.getenv("CI_PIPELINE_IID", "").strip()),
        ("GITHUB_RUN_NUMBER", os.getenv("GITHUB_RUN_NUMBER", "").strip()),
        ("CI_JOB_ID", os.getenv("CI_JOB_ID", "").strip()),
    ]
    for name, value in candidates:
        if value.isdigit():
            return int(value), f"env({name})"
    return None


def _next_local_counter(project_dir: Path) -> tuple[int, str]:
    counter_file = project_dir / ".build_counter"
    current = 0
    if counter_file.exists():
        raw = counter_file.read_text(encoding="utf-8").strip()
        if raw.isdigit():
            current = int(raw)

    next_value = current + 1
    counter_file.write_text(str(next_value), encoding="utf-8")
    return next_value, f"local_counter({counter_file.name})"


def _resolve_build_number(project_dir: Path) -> tuple[int, str]:
    resolved = _resolve_from_ci()
    if resolved is not None:
        return resolved
    return _next_local_counter(project_dir)


Import("env")

project_dir = Path(env.subst("$PROJECT_DIR"))
build_number, build_source = _resolve_build_number(project_dir)

# Unified value for firmware compile-time and post-build scripts.
env.Append(CPPDEFINES=[("APP_BUILD_NUMBER", build_number)])
env["APP_BUILD_NUMBER_RESOLVED"] = str(build_number)
env["APP_BUILD_SOURCE"] = build_source

# Make it visible for scripts relying on process environment.
os.environ["BUILD_NUMBER"] = str(build_number)
os.environ["BUILD_SOURCE"] = build_source

print(f"[version] Build number: {build_number} ({build_source})")
