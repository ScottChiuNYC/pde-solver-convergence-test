#!/usr/bin/env python3
"""Supplemental Linux/clang/g++ self-containment check; canonical CMake remains authoritative."""
from pathlib import Path
import argparse
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--include-root", required=True, type=Path,
                        help="Parent of pde_software_replication_core, in source or installed prefix")
    parser.add_argument("--compiler", default="c++")
    args = parser.parse_args()
    include_root = args.include_root.resolve()
    public_root = include_root / "pde_software_replication_core"
    umbrella = public_root / "PDE.h"
    if not umbrella.is_file():
        raise SystemExit(f"Missing required umbrella: {umbrella}")
    headers = sorted(path for path in public_root.rglob("*")
                     if path.is_file() and path.suffix in {".h", ".hpp", ".hh"})
    if not headers:
        raise SystemExit("No public headers discovered")
    with tempfile.TemporaryDirectory(prefix="c2c-header-check-") as temporary:
        source = Path(temporary) / "self_contained.cpp"
        for header in headers:
            spelling = header.relative_to(include_root).as_posix()
            # Include twice in the SAME TU to expose missing/incorrect guards.
            source.write_text(f"#include <{spelling}>\n#include <{spelling}>\nint main() {{ return 0; }}\n")
            command = [args.compiler, "-std=c++17", "-Wall", "-Wextra", "-pedantic-errors",
                       "-fsyntax-only", "-I", str(include_root), str(source)]
            print("HEADER", spelling, flush=True)
            subprocess.run(command, check=True)
    print(f"PASS: {len(headers)} headers compile independently and twice in C++17")


if __name__ == "__main__":
    main()
