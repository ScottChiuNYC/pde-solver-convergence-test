from __future__ import annotations

import importlib.util
from pathlib import Path
import re
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "scripts" / "validate_pdf_build.py"
SPEC = importlib.util.spec_from_file_location("validate_pdf_build", MODULE_PATH)
assert SPEC and SPEC.loader
validate_pdf_build = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(validate_pdf_build)


def has_forbidden_text(text: str) -> bool:
    return any(
        re.search(pattern, text, re.IGNORECASE)
        for pattern in validate_pdf_build.FORBIDDEN_PDF_TEXT_PATTERNS
    )


class ValidatePdfBuildTest(unittest.TestCase):
    def test_detects_nbsphinx_math_leakage_variants(self) -> None:
        # Build the deliberately forbidden samples from fragments. CodeBinder includes
        # this test source in the repository PDF, so literal bad samples here would
        # make the final-PDF guardrail correctly reject its own test fixture.
        bad_samples = tuple(
            "".join(parts)
            for parts in (
                ("alpha : ", "nbsphinx", "-math : text{level}"),
                ("alpha ", "nbsphinx", " -- math : text{level}"),
                ("alpha ", "nbsphinx", " — math : text{level}"),
            )
        )
        for sample in bad_samples:
            self.assertTrue(has_forbidden_text(sample), sample)

    def test_allows_normal_math_text(self) -> None:
        self.assertFalse(has_forbidden_text("alpha ↔ volatility level, rho ↔ skew"))

    def test_allows_documentation_of_token(self) -> None:
        self.assertFalse(has_forbidden_text("the nbsphinx-math token is forbidden in rendered equations"))


if __name__ == "__main__":
    unittest.main()
