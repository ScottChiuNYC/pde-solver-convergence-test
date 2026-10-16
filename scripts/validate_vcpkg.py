"""Install the local library port and run an isolated, relocated C++ consumer."""
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

from bootstrap import run, tool_environment


def validate_vcpkg(project, env):
    project = Path(project).resolve()
    vcpkg_root = Path(env["VCPKG_ROOT"])
    vcpkg = vcpkg_root / ("vcpkg.exe" if os.name == "nt" else "vcpkg")
    python_package = (project / "pyproject.toml").exists()
    baseline = json.loads((project / "vcpkg-configuration.json").read_text())["default-registry"]
    windows = sys.platform == "win32"
    base_triplet = "x64-windows" if windows else "x64-linux"
    linkages = (("dynamic",) if windows else ("static",)) if python_package else ("static", "dynamic")
    with tempfile.TemporaryDirectory(prefix="vcpkg-consumer-") as directory:
        # Resolve Windows 8.3 aliases before comparing installed CMake paths.
        work = Path(directory).resolve()
        triplets = work / "triplets"
        triplets.mkdir()
        # Host helpers need the same pinned Ninja setup as the existing presets.
        shutil.copy2(Path(env["PROJECT_DEV_TOOLS"]) / "triplets" / f"{base_triplet}.cmake",
                     triplets / f"{base_triplet}.cmake")
        for linkage in linkages:
            expected_linkage = "static" if python_package else linkage
            triplet = f"x64-packaging-{linkage}"
            content = (
                f'include("${{VCPKG_ROOT_DIR}}/triplets/{base_triplet}.cmake")\n'
                f'set(VCPKG_LIBRARY_LINKAGE {linkage})\n'
                'set(NINJA "$ENV{NINJA}")\n'
                'list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DCMAKE_MAKE_PROGRAM:FILEPATH=${NINJA}")\n'
            )
            (triplets / f"{triplet}.cmake").write_text(content)
            consumer = work / linkage
            shutil.copytree(project / "tests/vcpkg-consumer", consumer)
            (consumer / "vcpkg-configuration.json").write_text(json.dumps({"default-registry": baseline}))
            installed = consumer / "vcpkg_installed"
            # The recipe reads mutable local sources; never reuse a binary cache.
            run([vcpkg, "install", f"--x-manifest-root={consumer}", f"--x-install-root={installed}",
                 f"--overlay-ports={project / 'vcpkg-ports'}", f"--overlay-triplets={triplets}",
                 f"--triplet={triplet}", f"--x-buildtrees-root={consumer / 'buildtrees'}",
                 f"--x-packages-root={consumer / 'packages'}", "--binarysource=clear"], cwd=consumer, env=env)
            info = installed / "vcpkg/info"
            assert not list(info.glob("gtest_*")) and not list(info.glob("pkgconf_*"))
            if python_package:
                assert not list(installed.rglob("*.pyd")) and not list(installed.rglob("*.so"))
                # No Python detection or pybind11 FetchContent occurred in the port.
                port = json.loads((consumer / "vcpkg.json").read_text())["dependencies"][0]
                for cache in (consumer / "buildtrees" / port).glob("*/CMakeCache.txt"):
                    text = cache.read_text()
                    assert "BUILD_PYTHON_BINDINGS:BOOL=OFF" in text
                    keys = [line.split(":", 1)[0] for line in text.splitlines()
                            if line and not line.startswith(("#", "//"))]
                    assert not [key for key in keys if key != "BUILD_PYTHON_BINDINGS"
                                and ("python" in key.lower() or "pybind11" in key.lower())]
            relocated = consumer / "relocated-installed"
            installed.rename(relocated)
            prefix = relocated / triplet
            # Hide source headers while consuming; a source-tree include leak must fail.
            headers = project / ("cpp/include" if python_package else "include")
            hidden = headers.with_name("include.consumer-hidden")
            headers.rename(hidden)
            try:
                for configuration in ("Release", "Debug"):
                    build = consumer / f"build-{configuration}"
                    options = ["-G", "Visual Studio 17 2022", "-A", "x64"] if windows else [
                        "-G", "Ninja", f"-DCMAKE_MAKE_PROGRAM:FILEPATH={env['NINJA']}"]
                    run(["cmake", "-S", consumer, "-B", build, *options,
                         f"-DCMAKE_BUILD_TYPE={configuration}",
                         f"-DCMAKE_TOOLCHAIN_FILE={vcpkg_root / 'scripts/buildsystems/vcpkg.cmake'}",
                         f"-DVCPKG_INSTALLED_DIR={relocated}", f"-DVCPKG_TARGET_TRIPLET={triplet}",
                         "-DVCPKG_MANIFEST_INSTALL=OFF", "-DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF",
                         "-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF",
                         f"-DEXPECTED_PREFIX={prefix.as_posix()}", f"-DEXPECTED_LINKAGE={expected_linkage}"],
                        cwd=consumer, env=env)
                    run(["cmake", "--build", build, "--config", configuration], cwd=consumer, env=env)
                    runtime_env = env.copy()
                    runtime_bin = prefix / ("debug/bin" if configuration == "Debug" else "bin")
                    runtime_env["PATH"] = str(runtime_bin) + os.pathsep + env["PATH"]
                    run(["ctest", "--test-dir", build, "-C", configuration, "--no-tests=error", "--verbose"],
                        cwd=consumer, env=runtime_env)
            finally:
                hidden.rename(headers)
            print(f"vcpkg consumer passed: {linkage} triplet -> {expected_linkage} library, Release + Debug, relocated install, source headers hidden.", flush=True)
        downloads = vcpkg_root / "downloads"
        assert not [p for d in (downloads, downloads / "tools") if d.exists() for p in d.iterdir()
                    if p.name.lower() == "pkgconf" or p.name.lower().startswith("pkgconf-")]


if __name__ == "__main__":
    project = Path(__file__).resolve().parents[1]
    _, env, _ = tool_environment(project)
    validate_vcpkg(project, env)
