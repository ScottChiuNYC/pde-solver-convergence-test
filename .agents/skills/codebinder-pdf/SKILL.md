---
name: codebinder-pdf
description: Use for repository-wide CodeBinder PDF generation, validation, GitHub Actions publication, optional Google Drive upload, and CodeBinder-specific Markdown/math rendering guardrails. Do not use for ordinary PDF/OCR tasks; use the routed external PDF skill instead.
compatibility: GitHub Actions is the canonical ART publication backend. Local environments may use the bundled validation/build helpers; a full local PDF build additionally requires the CodeBinder/Sphinx/LaTeX toolchain. Local environments are not required to reproduce GitHub publication parity.
---

# CodeBinder PDF

This skill owns the ART-provided repository PDF pipeline.

Use progressive disclosure:

- For publication behavior, GitHub Actions, artifacts, and Google Drive configuration, read `references/publication.md`.
- For Markdown math rules and the rendered-math regression guardrail, read `references/math-rendering.md`.
- Deterministic helpers live under `scripts/`.
- Regression tests for the helpers live under `tests/`.

The canonical GitHub workflow is:

`.github/workflows/publish-codebinder-pdf.yml`

## Backend model

The skill itself is portable across agent runtimes. Local environments may read the skill and run bundled validation helpers, and they may perform a full PDF build when the required CodeBinder/Sphinx/LaTeX dependencies are available.

ART does not require local publication parity. The supported publication path is GitHub Actions, which pulls an immutable, digest-pinned CodeBinder/TeX builder image from GHCR. The image owns heavy toolchain dependencies; this skill owns repository-specific behavior, validation, and publication rules.

The same builder image is intended to remain usable later on a persistent self-hosted/VPS runner. Do not force a local environment to install or reproduce the full publication stack merely because this skill is present.

## Boundary

This skill is for the repository-wide CodeBinder artifact. It does not replace a general PDF/OCR skill.

Do not add unrelated repository operating protocol here. Keep CodeBinder-specific behavior inside this skill and keep `AGENTS.md` as a router.
