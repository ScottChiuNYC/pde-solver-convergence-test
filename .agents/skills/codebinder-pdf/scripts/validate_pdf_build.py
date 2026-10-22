#!/usr/bin/env python3
"""Fail closed when a Sphinx/LaTeX PDF build is structurally invalid."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import sys

FATAL_LOG_PATTERNS = (
    r"! LaTeX Error:",
    r"Emergency stop",
    r"Fatal error occurred",
    r"Undefined control sequence",
)

# Match leaked nbsphinx math-role syntax, not prose that merely documents the token.
FORBIDDEN_PDF_TEXT_PATTERNS = (
    r"nbsphinx\s*[-–—]+\s*math\s*:",
    r"nbsphinx-math\s*:",
)


def pdf_pages(path: Path) -> int:
    completed = subprocess.run(["pdfinfo", str(path)], text=True, capture_output=True, check=False)
    if completed.returncode != 0:
        raise RuntimeError(completed.stderr.strip() or "pdfinfo failed")
    match = re.search(r"^Pages:\s+(\d+)\s*$", completed.stdout, re.MULTILINE)
    if not match:
        raise RuntimeError("pdfinfo did not report page count")
    return int(match.group(1))


def pdf_text(path: Path) -> str:
    completed = subprocess.run(
        ["pdftotext", str(path), "-"], text=True, capture_output=True, check=False
    )
    if completed.returncode != 0:
        raise RuntimeError(completed.stderr.strip() or "pdftotext failed")
    return completed.stdout


def toc_entries(path: Path) -> int:
    if not path.is_file():
        return 0
    text = path.read_text(encoding="utf-8", errors="replace")
    return len(re.findall(r"\\contentsline\s*\{", text))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pdf", required=True)
    parser.add_argument("--toc", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--min-entries", type=int, default=1)
    parser.add_argument("--min-pages", type=int, default=1)
    args = parser.parse_args()

    pdf, toc, log = map(Path, (args.pdf, args.toc, args.log))
    errors: list[str] = []
    if not pdf.is_file() or pdf.stat().st_size < 1024:
        errors.append(f"missing or trivial PDF: {pdf}")
    else:
        try:
            pages = pdf_pages(pdf)
            if pages < args.min_pages:
                errors.append(f"PDF has {pages} page(s); expected at least {args.min_pages}")
        except RuntimeError as exc:
            errors.append(str(exc))
        try:
            extracted = pdf_text(pdf)
            for pattern in FORBIDDEN_PDF_TEXT_PATTERNS:
                if re.search(pattern, extracted, re.IGNORECASE):
                    errors.append(f"forbidden rendered-text pattern: {pattern}")
        except RuntimeError as exc:
            errors.append(str(exc))
    entries = toc_entries(toc)
    if entries < args.min_entries:
        errors.append(f"TOC has {entries} entries; expected at least {args.min_entries}")
    if not log.is_file():
        errors.append(f"missing LaTeX log: {log}")
    else:
        text = log.read_text(encoding="utf-8", errors="replace")
        for pattern in FATAL_LOG_PATTERNS:
            if re.search(pattern, text, re.IGNORECASE):
                errors.append(f"fatal log pattern: {pattern}")
    if errors:
        print("PDF validation failed:\n- " + "\n- ".join(errors), file=sys.stderr)
        return 1
    print(f"PDF validation passed: {pdf} ({pdf.stat().st_size} bytes, {entries} TOC entries)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
