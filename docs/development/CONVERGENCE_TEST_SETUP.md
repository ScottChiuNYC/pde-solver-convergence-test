# Convergence test preparation

Prepared on 2026-10-16 as a fresh starting point for the second convergence
skill, which takes a frozen domain contract through implementation.

## Project skeleton

- Generator: Copier 9.19.1, using `copier copy`.
- Template: <https://github.com/ScottChiuNYC/copier-pypackage-cpp>.
- Exact template revision: `89ce8d23659ecc0a929284c535e6cd4c642e44d7`.
- Verified template CI: <https://github.com/ScottChiuNYC/copier-pypackage-cpp/actions/runs/38778240104> (success).
- Copier records this revision as `v1.0.0-1-g89ce8d2` in `.copier-answers.yml`.
- Project name/slug: `PDE Solver Convergence Test` / `pde-solver-convergence-test`.
- Module: `pde_software_replication`; C++ port: `pde-software-replication-core`.
- Project Skeleton Contract: [version 1.1](PROJECT_SKELETON_CONTRACT.md), supplied by the template.

The frozen domain contract requires the public umbrella spelling
`<pde_software_replication_core/PDE.h>`. Keeping the original module name
preserves that requirement without changing the contract or creating a second
public include root.

## Frozen domain contract

- Local authority: [essence.md](essence.md), copied without edits.
- Source repository: `ScottChiuNYC/pde-solver-replication`.
- Source commit: `0a5918a6ae93fee4633fabf2fcec62a74174de83`.
- Exact source: <https://github.com/ScottChiuNYC/pde-solver-replication/blob/0a5918a6ae93fee4633fabf2fcec62a74174de83/docs/development/essence.md>.
- Frozen Git blob: `4f8c036f9e916fcacce37ee56c72566af9f5b8ed`.
- Content SHA-256: `a8ef1e0cff9848b8fce3ee865b9171ae5587b405e525b85974c9faa9d442b436`.
- Size: 26,515 bytes.
- Status carried by the source: `FROZEN / IMPLEMENTATION_READY`; unresolved owner decisions: 0.
- Audited authority commit recorded by the source: `23c0e26209bfd3f992f556dbc6ad97822ca2730b`.
- Audited Essence blob recorded by the source: `4c88b6656bba9adc71907cadaf4de1da8d64f3a9`.
- Reconciled contract PASS: <https://github.com/ScottChiuNYC/pde-solver-replication/issues/13>.
- Source audit journal: <https://github.com/ScottChiuNYC/pde-solver-replication/blob/0a5918a6ae93fee4633fabf2fcec62a74174de83/docs/development/ESSENCE_AUDIT.md>.

The source's freeze metadata is retained verbatim. Its audit references describe
the source contract audit; this preparation does not perform another audit.

Only the frozen Essence was copied from the replication project. Solver code,
domain tests, implementation reports, and implementation review results were
not copied. The generated Hello World implementation and baseline tests are
the starting code. No domain implementation or execution of the second skill
is part of this preparation.
