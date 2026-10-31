# Agent operating guide

This repository uses **ART (Agentic Repo Template)** as a small skill router plus repository-native agent infrastructure.

Keep this file concise. Do not preload every referenced skill. Classify the task first, then read only the relevant skill.

## Skill routing

### Repository contribution / pull requests

For branch, commit, pull-request, review, and repository contribution workflow, use GitHub's `make-repo-contribution` skill:

https://github.com/github/awesome-copilot/blob/main/skills/make-repo-contribution/SKILL.md

Owner policy for ART-generated repositories: once all applicable validation and repository requirements pass, agents are pre-authorized to **squash merge** their verified PRs unless the current task explicitly says not to merge or another repository rule blocks merge. Do not wait for a redundant merge approval.

### Implementation Contract Convergence

When the owner explicitly asks to converge implementation intent or a draft Essence into a FROZEN / implementation-ready contract, use:

https://github.com/ScottChiuNYC/implementation-convergence/blob/main/skills/implementation-contract-convergence/SKILL.md

This workflow is explicitly invoked; do not start it merely because an Essence exists.

### Contract-to-Code Convergence

When the owner explicitly asks to implement a FROZEN / implementation-ready Essence through the C2C workflow, use:

https://github.com/ScottChiuNYC/implementation-convergence/blob/main/skills/contract-to-code-convergence/SKILL.md

This workflow is explicitly invoked; do not start it merely because a frozen contract exists.

### General PDF work and OCR

For ordinary PDF reading, extraction, editing, transformation, or OCR of scanned PDFs, use Anthropic's PDF skill:

https://github.com/anthropics/skills/blob/main/skills/pdf/SKILL.md

Do not rebuild generic PDF/OCR machinery inside this repository when that skill is sufficient.

### CodeBinder repository PDF

For repository-wide CodeBinder PDF generation, validation, publication, Google Drive upload, or CodeBinder-specific Markdown/math rendering behavior, use the repo-local skill:

`.agents/skills/codebinder-pdf/SKILL.md`

This local skill takes precedence over the general PDF skill for the repository CodeBinder pipeline.

## Repository provenance and ownership

Before structural or infrastructure changes, reconstruct how this repository was assembled:

- inspect every `.copier-answers.*.yml` file present;
- `.copier-answers.art.yml` identifies the ART revision that owns this skill router and PDF publication layer;
- `.copier-answers.pypackage-cpp.yml`, when present, identifies the independent Python/C++ skeleton revision;
- read `docs/development/PROJECT_SKELETON_CONTRACT.md` when present for project-local build, packaging, dependency, layout, and test conventions;
- treat `.codebinderignore` as project-owned and `.codebinderignore.d/*` as composable owner-specific fragments; do not consolidate template-owned fragments into one shared generated file;
- the PDF workflow intentionally pins its builder image by immutable GHCR digest. Do not replace that authority with a floating tag.

The broader ecosystem and change-propagation model is documented at:

https://github.com/ScottChiuNYC/agentic-repo-template/blob/main/docs/ecosystem-architecture.md

## Project coding conventions

When `docs/development/PROJECT_SKELETON_CONTRACT.md` is present, it owns language-level coding standards, including Google C++ and PEP 8 where applicable and any project-specific compatibility exceptions. Read it for declarations, source, tests and examples, including when using ICC/C2C. ART does not override project-local policy. Do not run automatic formatters, add mandatory formatter-only CI, or mass-reformat unrelated code. Review modified lines and preserve intentional mathematical alignment.

## Minimal operating rules

- Prefer mature existing skills and native runtime tools over repository-specific reinvention.
- Read current repository authority before mutating files.
- Work on an isolated branch rather than directly on `main`, except when bootstrapping an otherwise empty repository makes an initial commit unavoidable.
- For narrow local edits, prefer the runtime's native patch/edit primitive when available.
- For connector-only GitHub edits where the API replaces a whole file, bind the write to the current file/blob identity when supported, use an isolated branch, read back the result, and inspect the exact diff before merge.
- If the outcome of an external mutation is unknown, observe current state before retrying. Do not create duplicate commits, branches, PRs, or side effects by guessing.
