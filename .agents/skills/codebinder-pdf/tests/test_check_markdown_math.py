from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "scripts" / "check_markdown_math.py"
SPEC = importlib.util.spec_from_file_location("check_markdown_math", MODULE_PATH)
assert SPEC and SPEC.loader
check_markdown_math = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check_markdown_math)


class CheckMarkdownMathTest(unittest.TestCase):
    def validate_text(self, text: str) -> list[str]:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "test.md"
            path.write_text(text, encoding="utf-8")
            return check_markdown_math.validate(path)

    def test_accepts_dollar_display_math(self) -> None:
        errors = self.validate_text("Before\n\n$$\n\\alpha \\leftrightarrow \\text{level}\n$$\n")
        self.assertEqual(errors, [])

    def test_rejects_backslash_display_math(self) -> None:
        errors = self.validate_text("Before\n\n\\[\n\\alpha \\leftrightarrow \\text{level}\n\\]\n")
        self.assertTrue(any("unsupported display-math delimiter" in error and r"\[" in error for error in errors))
        self.assertTrue(any("unsupported display-math delimiter" in error and r"\]" in error for error in errors))

    def test_normalizes_backslash_display_math_without_source_semantic_change(self) -> None:
        source = "Before\n\n\\[\n\\alpha + \\beta\n\\]\nAfter\n"
        normalized, count = check_markdown_math.normalize_unsupported_display_math(source)
        self.assertEqual(count, 2)
        self.assertEqual(normalized, "Before\n\n$$\n\\alpha + \\beta\n$$\nAfter\n")
        self.assertEqual(len(normalized), len(source))

    def test_normalizes_texttt_underscore_for_latex(self) -> None:
        source = r"\texttt{numeric_limits<double>::epsilon()}"
        normalized, count = check_markdown_math.normalize_texttt_underscores(source)
        self.assertEqual(count, 1)
        self.assertEqual(
            normalized,
            r"\texttt{numeric\_limits<double>::epsilon()}",
        )

    def test_texttt_normalization_ignores_inline_code(self) -> None:
        source = r"`\texttt{code_name}` and \texttt{real_name}"
        normalized, count = check_markdown_math.normalize_texttt_underscores(source)
        self.assertEqual(count, 1)
        self.assertIn(r"`\texttt{code_name}`", normalized)
        self.assertIn(r"\texttt{real\_name}", normalized)

    def test_normalization_ignores_code_and_escaped_delimiters(self) -> None:
        source = (
            "`\\[inline\\]`\n\n"
            "```text\n\\[fenced\\]\n```\n\n"
            "    \\[indented\\]\n\n"
            "\\\\[escaped\\\\]\n\n"
            "\\[real\\]\n"
        )
        normalized, count = check_markdown_math.normalize_unsupported_display_math(source)
        self.assertEqual(count, 2)
        self.assertIn("`\\[inline\\]`", normalized)
        self.assertIn("\\[fenced\\]", normalized)
        self.assertIn("    \\[indented\\]", normalized)
        self.assertIn("\\\\[escaped\\\\]", normalized)
        self.assertIn("$$real$$", normalized)

    def test_ignores_delimiters_in_code(self) -> None:
        errors = self.validate_text(
            "`\\[literal\\]`\n\n"
            "``\\[literal with ` backtick\\]``\n\n"
            "```text\n\\[literal\\]\n```\n\n"
            "~~~~text\n\\[literal\\]\n~~~~\n"
        )
        self.assertEqual(errors, [])

    def test_ignores_indented_code_blocks(self) -> None:
        errors = self.validate_text(
            "Before\n\n"
            "    $implementation-contract-convergence\n"
            "    \\[literal\\]\n"
            "\t$HOME\n"
            "\n"
            "After\n"
        )
        self.assertEqual(errors, [])

    def test_indented_code_does_not_hide_following_prose(self) -> None:
        errors = self.validate_text("    $command\n\n\\[bad\\]\n")
        self.assertEqual(len(errors), 2)

    def test_escaped_and_mismatched_backticks_do_not_hide_delimiters(self) -> None:
        escaped = self.validate_text("\\` before \\[bad\\] after \\`\\n")
        mismatched = self.validate_text("prefix ``` \\[bad\\] `` suffix\n")
        self.assertEqual(len(escaped), 2)
        self.assertEqual(len(mismatched), 2)

    def test_ignores_multiline_inline_code_and_nested_fence_examples(self) -> None:
        errors = self.validate_text(
            "``\n\\[literal\\]\n``\n\n"
            "````markdown\n```text\n\\[literal\\]\n```\n````\n"
        )
        self.assertEqual(errors, [])


if __name__ == "__main__":
    unittest.main()
