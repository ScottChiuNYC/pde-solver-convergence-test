#!/usr/bin/env python3
"""Validate Markdown math delimiters and normalize legacy display math in staging copies."""

from __future__ import annotations

from pathlib import Path
import re
import sys

FENCE_RE = re.compile(r"^\s*(`{3,}|~{3,})(.*)$")
UNSUPPORTED_DISPLAY_MATH_RE = re.compile(r"(?<!\\)\\([\[\]])")


def strip_escaped(text: str) -> str:
    return re.sub(r"\\.", "", text)


def _is_escaped(text: str, index: int) -> bool:
    backslashes = 0
    cursor = index - 1
    while cursor >= 0 and text[cursor] == "\\":
        backslashes += 1
        cursor -= 1
    return backslashes % 2 == 1


def _mask_indented_code(text: str) -> str:
    """Mask CommonMark-style indented code blocks while preserving positions."""

    masked: list[str] = []
    in_block = False
    previous_blank = True

    for raw_line in text.splitlines(keepends=True):
        body = raw_line.rstrip("\r\n")
        newline = raw_line[len(body) :]
        is_blank = not body.strip()
        is_indented = body.startswith("    ") or body.startswith("\t")

        if in_block:
            if is_indented or is_blank:
                masked.append(" " * len(body) + newline)
                continue
            in_block = False

        if is_indented and previous_blank:
            in_block = True
            masked.append(" " * len(body) + newline)
            previous_blank = False
            continue

        masked.append(raw_line)
        previous_blank = is_blank

    return "".join(masked)


def _mask_fenced_code(text: str) -> tuple[str, bool]:
    masked: list[str] = []
    fence_marker = ""

    for raw_line in text.splitlines(keepends=True):
        body = raw_line.rstrip("\r\n")
        newline = raw_line[len(body) :]
        match = FENCE_RE.match(body)

        if fence_marker:
            if match:
                marker, info = match.groups()
                if (
                    marker[0] == fence_marker[0]
                    and len(marker) >= len(fence_marker)
                    and not info.strip()
                ):
                    fence_marker = ""
            masked.append(" " * len(body) + newline)
            continue

        if match:
            fence_marker = match.group(1)
            masked.append(" " * len(body) + newline)
        else:
            masked.append(raw_line)

    return "".join(masked), bool(fence_marker)


def _mask_inline_code(text: str) -> str:
    """Mask valid CommonMark backtick spans while preserving positions."""

    runs: list[tuple[int, int]] = []
    cursor = 0
    while cursor < len(text):
        if text[cursor] != "`":
            cursor += 1
            continue
        end = cursor + 1
        while end < len(text) and text[end] == "`":
            end += 1
        runs.append((cursor, end))
        cursor = end

    characters = list(text)
    run_index = 0
    while run_index < len(runs):
        start, opener_end = runs[run_index]
        if _is_escaped(text, start):
            run_index += 1
            continue

        marker_length = opener_end - start
        closing_index = run_index + 1
        while closing_index < len(runs):
            close_start, close_end = runs[closing_index]
            if close_end - close_start == marker_length:
                for index in range(start, close_end):
                    if characters[index] not in "\r\n":
                        characters[index] = " "
                run_index = closing_index + 1
                break
            closing_index += 1
        else:
            run_index += 1

    return "".join(characters)


def _mask_protected_regions(text: str) -> tuple[str, bool]:
    masked = _mask_indented_code(text)
    masked, unclosed_fence = _mask_fenced_code(masked)
    return _mask_inline_code(masked), unclosed_fence


def normalize_unsupported_display_math(text: str) -> tuple[str, int]:
    """Convert unescaped \\[ / \\] outside code to $$ without changing length."""

    masked, _ = _mask_protected_regions(text)
    characters = list(text)
    replacements = 0

    for match in UNSUPPORTED_DISPLAY_MATH_RE.finditer(masked):
        start, end = match.span()
        # Both legacy delimiters and "$$" are two characters, so all later
        # character offsets remain stable.
        characters[start:end] = ["$", "$"]
        replacements += 1

    return "".join(characters), replacements


def normalize_path(path: Path) -> int:
    text = path.read_text(encoding="utf-8")
    normalized, replacements = normalize_unsupported_display_math(text)
    if replacements:
        path.write_text(normalized, encoding="utf-8")
    return replacements


def validate(path: Path) -> list[str]:
    errors: list[str] = []
    text = path.read_text(encoding="utf-8")
    text, in_fence = _mask_protected_regions(text)
    display_open = False

    for lineno, raw in enumerate(text.splitlines(), 1):
        line = raw
        for match in UNSUPPORTED_DISPLAY_MATH_RE.finditer(line):
            delimiter = "\\" + match.group(1)
            errors.append(
                f"{path}:{lineno}: unsupported display-math delimiter '{delimiter}'; use '$$' blocks"
            )

        line = strip_escaped(line)
        display_count = line.count("$$")
        if display_count:
            if display_count > 2:
                errors.append(f"{path}:{lineno}: too many display-math delimiters on one line")
            if display_count % 2 == 1:
                display_open = not display_open
            line = line.replace("$$", "")

        if not display_open:
            singles = line.count("$")
            if singles % 2:
                errors.append(f"{path}:{lineno}: unmatched inline '$' delimiter")

    if in_fence:
        errors.append(f"{path}: unclosed fenced code block")
    if display_open:
        errors.append(f"{path}: unclosed '$$' display-math block")
    return errors


def main(argv: list[str]) -> int:
    args = list(argv[1:])
    normalize = bool(args and args[0] == "--normalize")
    if normalize:
        args = args[1:]

    paths = [Path(arg) for arg in args]
    if not paths:
        mode = " --normalize" if normalize else ""
        print(
            f"usage: check_markdown_math.py{mode} <file.md> [file.md ...]",
            file=sys.stderr,
        )
        return 2

    errors: list[str] = []
    normalized = 0
    validated = 0
    for path in paths:
        if path.suffix.lower() != ".md":
            continue
        try:
            if normalize:
                normalized += normalize_path(path)
            errors.extend(validate(path))
            validated += 1
        except (OSError, UnicodeDecodeError) as exc:
            errors.append(f"{path}: {exc}")

    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1

    if normalize:
        print(
            f"normalized {normalized} legacy display-math delimiter(s); "
            f"validated {validated} Markdown file(s)"
        )
    else:
        print(f"validated {validated} Markdown file(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
