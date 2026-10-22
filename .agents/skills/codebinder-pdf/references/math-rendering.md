# Markdown math rendering guardrail

The CodeBinder -> nbsphinx -> Sphinx path has a known failure mode: LaTeX-style display delimiters `\\[ ... \\]` can leak internal `nbsphinx-math` markup into reader-visible PDF text.

ART must not require repository authority to be rewritten merely for publication compatibility. This matters especially for immutable or frozen contract artifacts.

ART therefore preserves two independent protections:

1. The workflow builds a tracked-files-only staging tree and normalizes unescaped `\\[` and `\\]` outside code to `$$` **only in that staging copy**. Repository source files remain unchanged.
2. Final-PDF validation extracts rendered text and fails if leaked `nbsphinx-math` role syntax is present.

The deterministic implementations are:

- `scripts/check_markdown_math.py` — validation plus `--normalize` staging mode;
- `scripts/validate_pdf_build.py` — final rendered-text validation.

Authors may still prefer `$$ ... $$` for new Markdown, but legacy `\\[ ... \\]` is a publication-compatibility concern rather than a reason to mutate otherwise authoritative repository text.
