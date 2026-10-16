"""Configure, build and test the generated project using its canonical presets."""
from pathlib import Path
import argparse
import json
import os
import sys
import tempfile
import venv
import tomllib

from bootstrap import python_in, run, tool_environment
from validate_vcpkg import validate_vcpkg


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wheel", action="store_true", help="Also build sdist/wheel and import the installed wheel in a clean environment.")
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    tools = Path(os.environ.get("PROJECT_DEV_TOOLS", project / ".dev-tools"))
    selected = tools / "vcpkg-root.txt"
    if selected.exists():
        os.environ["VCPKG_ROOT"] = selected.read_text(encoding="utf-8")
    py, env, options = tool_environment(project, tools)
    windows = sys.platform == "win32"
    configure = "windows-vcpkg" if windows else "linux-vcpkg-release"
    build = "windows-release" if windows else "linux-release"
    tests = "windows-release-tests" if windows else "linux-release-tests"
    python_package = (project / "pyproject.toml").exists()
    if python_package:
        run(["poetry", "install", "--no-root", "--no-ansi"], cwd=project, env=env)
        options += [f"-DPython_EXECUTABLE:FILEPATH={py}"]
    run(["cmake", "--preset", configure, *options], cwd=project, env=env)
    installed = list((project / "build").glob("*/vcpkg_installed/vcpkg/info/pkgconf_*"))
    downloads = Path(env["VCPKG_ROOT"], "downloads")
    acquired = [p for d in (downloads, downloads / "tools") if d.exists() for p in d.iterdir() if p.name.lower() == "pkgconf" or p.name.lower().startswith("pkgconf-")]
    if installed or acquired:
        raise SystemExit(f"pkgconf unexpectedly acquired: {installed + acquired}")
    run(["cmake", "--build", "--preset", build], cwd=project, env=env)
    run(["ctest", "--preset", tests, "--no-tests=error", "--verbose"], cwd=project, env=env)
    if python_package:
        env["PYTHONPATH"] = str(project / "src")
        run(["poetry", "run", "pytest", "tests/python"], cwd=project, env=env)
    if args.wheel:
        if not python_package:
            raise SystemExit("--wheel applies to the Python + C++ template only.")
        # The default build also rebuilds the wheel from the unpacked sdist.
        # -I prevents the project build.py hook from shadowing the build frontend.
        run([py, "-I", "-m", "build", "--no-isolation", project], cwd=project.parent, env=env)
        wheels = list((project / "dist").glob("*.whl"))
        assert len(wheels) == 1 and not wheels[0].name.endswith("none-any.whl"), wheels
        name = tomllib.loads((project / "pyproject.toml").read_text(encoding="utf-8"))["tool"]["poetry"]["name"]
        with tempfile.TemporaryDirectory(prefix="wheel-install-") as directory:
            clean = Path(directory) / "venv"
            venv.EnvBuilder(with_pip=True).create(clean)
            wheel_py = python_in(clean)
            run([wheel_py, "-m", "pip", "install", "--no-deps", wheels[0]], cwd=directory, env=env)
            code = f"import {name} as package; assert package.hello() == 'Hello, World!'; print(package.__file__)"
            run([wheel_py, "-I", "-c", code], cwd=directory, env=env)
    validate_vcpkg(project, env)
    print("Project validation passed.")


if __name__ == "__main__":
    main()
