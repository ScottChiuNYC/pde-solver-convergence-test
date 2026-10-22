#!/usr/bin/env python3
"""Disable Sphinx PDF bookmarks in generated conf.py as a LaTeX fallback."""

from pathlib import Path

path = Path("build/codebinder-source/conf.py")
if not path.is_file():
    raise SystemExit(f"missing generated Sphinx config: {path}")
with path.open("a", encoding="utf-8") as handle:
    handle.write(
        "\n# Fallback for LaTeX/PDF bookmark failures.\n"
        "latex_elements = dict(globals().get('latex_elements', {}))\n"
        "latex_elements['preamble'] = latex_elements.get('preamble', '') + r'''\n"
        "\\hypersetup{bookmarks=false}\n"
        "'''\n"
    )
print("disabled PDF bookmarks for fallback build")
