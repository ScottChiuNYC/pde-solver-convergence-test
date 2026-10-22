# Markdown math rendering guardrail

The CodeBinder -> nbsphinx -> Sphinx path has a known failure mode: LaTeX-style display delimiters `\\[ ... \\]` can leak internal `nbsphinx-math` markup into reader-visible PDF text.

ART therefore preserves two independent checks:

1. Markdown validation rejects unescaped `\\[` and `\\]` outside code and requires standalone `$$ ... $$` blocks for display math.
2. Final-PDF validation extracts rendered text and fails if leaked `nbsphinx-math` role syntax is present.

The deterministic implementations are:

- `scripts/check_markdown_math.py`
- `scripts/validate_pdf_build.py`

Do not weaken one check merely because the other exists.
