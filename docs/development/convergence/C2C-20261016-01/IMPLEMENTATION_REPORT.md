# Implementation evidence — C2C-20261016-01

Implementer: `/root/implementer`, 2026-10-16 UTC. This is implementation evidence for independent review, not final verification or merge authorization. No other PDE implementation, repository, experiment result, or prior review was inspected. No subagents were used.

## Identity and authority

The work started from acceptance-sealed HEAD `91c618694f8b5c847bd6d84e762f0d6d4bf68b2c` on `c2c/20261016-frozen-pde`; the pre-acceptance baseline is `17fae54f55ac67e56b18eec9f39242ea7d7786fd`. Domain implementation commit: `87a32549308a6b590c498b51bcbfa444a642c6ee`, tree `cb728040755462f8ddd3dded6bbfc6e49257e5da`. Integration-proof follow-up: `49ddf0c800a2958a5c76fedf9ebbc696ee7c7082`, tree `643ef1aabe574e1f2fcd134ddfed6c9e2da054c7`. This report is added subsequently; the dispatcher records the final transport/candidate SHA separately, avoiding self-reference.

Unchanged authority SHA256: Essence `a8ef1e0cff9848b8fce3ee865b9171ae5587b405e525b85974c9faa9d442b436`; Boundary v1.1 `9a714fe8fba12d3cae9047816bd3150c70c8a0994ce298d20023f853a74ef849`. The sealed manifest remains `857a5f26329ba7df04f5a87dd6ca458de92bf6a39ead0f780d912007ef21b3a9`, and index remains `6df9e8e2d83903192f85bda0959d4fd5e0b86bd2b4e7978c3b361296ffed2048`. All 15 indexed files pass `sha256sum -c`; no sealed assertion/oracle was changed or adjudication requested. Prior audit/provenance is preserved.

## Public-header map and implementation

Every path below is relative to `cpp/include/pde_software_replication_core/`.

| Logical unit | Physical header |
| --- | --- |
| Types | `Types.h` |
| Domain | `Domain.h` |
| Grid | `Grid.h` |
| Array | `Array.h` |
| Equation | `Equation.h` |
| Tridiagonal | `Tridiagonal.h` |
| Scheme::CrankNicholson1D | `Scheme/CrankNicholson1D.h` |
| Umbrella | `PDE.h` |

The umbrella includes all units. `detail/CheckedArithmetic.h` supplies inline private numeric helpers; the existing `hello.h` remains. All domain definitions are inline/in-class, guarded, and standard-library-only. Each arithmetic helper stores an operation into a volatile `double` before checking finiteness, preventing contraction or wider retained intermediates from concealing an overflow. No build flags weaken numerical checking.

Array copies stage before swapping, moves empty the source and preserve self-move, and checked access uses runtime errors. Grid cardinalities/endpoints are validated before spans, division, or allocations. The scheme retains const references, revalidates callback presence/types before scratch or callbacks, uses local solve storage, and evaluates only required samples. Thomas validates shape/aliasing/finite inputs, stages pivots/reduced RHS/solution, applies the original-row pivot rule immediately, and swaps the RHS only after finite complete substitution.

The existing static core, pybind11 FetchContent provider, native hello bindings/Python tests, presets, dependency baselines and Copier revision are preserved. Integration changes are limited to recursive public-header sdist inclusion, actual checkout revision injection in canonical validation, installed solver assertions/header probes, and explicit source-hidden/restoration assertions. There is no Python solver API or added numerical dependency.

## Chronological implementation gates

Complete command lines, timestamps, compiler output, test inventory and exits are in `/workspace/scratch/31534520b72e/implementation-logs/`.

| Gate (UTC start) | Evidence log | Result |
| --- | --- | --- |
| Helpers, 16:55:28 | `gate-1-helpers-pass.log` | 5 sealed public tests; Essence §6 C++17 compile-only before the solve body existed; every header separately/twice |
| Thomas, 16:56:07 | `gate-2-thomas.log` | All 5 shape/scaling/pivot/preservation/nonfinite tests passed before PDE stepping was wired |
| Stepping, 16:57:11 | `gate-3-stepping.log` | 8 callback/reference/concurrency tests, 3 arithmetic tests, direct heat-mode test for all 3 theta values |
| Numerical, 16:58:48 | `gate-4-numerics.log` | 7 numerical tests plus two-TU umbrella integration |
| Implementation regressions, 16:59:15 | `implementation-regressions.log` | Pivot checked before an overflowing reduced-RHS operation; exact affine solution with variable coefficients/nonunit coordinates; role/coordinate diagnostic context |
| Bootstrap, 16:58:53 | `bootstrap.log` | Pinned canonical bootstrap passed |
| Canonical | `canonical-87a3254.log`, `canonical-hidden-proof.log` | See completed validation below |

Preliminary gates were work-in-progress rooted at the acceptance HEAD, with source metadata describing that checkout. They are chronological gate evidence, not final candidate-SHA evidence. Complete canonical runs identify their actual committed source. One initial helper invocation used the wrong location for the prebuilt GTest main archive; its linker failure is retained in `gate-1-helpers.log` and corrected in the passing log. Focused pre-step gates used only the explicitly permitted baseline binary GTest package, not another source implementation.

## Acceptance traceability

| AC IDs | Implementation/evidence |
| --- | --- |
| AC001–002 | `Array.h` ownership, copy/swap, move and bounds; sealed `AcceptancePublic` array tests |
| AC003–004 | `Domain.h`, `Grid.h` endpoint/span/spacing/cardinality checks, owned domain and coordinate methods; sealed grid tests |
| AC005 | `Equation.h` defaults; scheme constructor/factory references and transactional `setTheta`; sealed defaults/traits test |
| AC006–010 | `Tridiagonal.h` shape/alias checks, exact original row scales, strict relative pivot threshold, prescribed checked recurrence and private commit; all sealed Thomas tests, plus pivot-first regression |
| AC011–015 | `validateConfiguration` before scratch, `sample` finite/positive checks without exception wrapping/retry; sealed every-role presence, sample-set, nonfinite and user-exception cases |
| AC016 | Scheme old endpoints and left-then-right new boundary corrections, including one interior row; sealed rational expected-value case |
| AC017 | Reference members and independent local arrays, no persistent cache; sealed mutation/recovery/repeat/copy/move test |
| AC018 | `scripts/validate.py` obtains actual 40-hex `git rev-parse HEAD` and injects `C2C_SOURCE_REVISION`; canonical binary64/rounding/underflow and compiler/build output |
| AC019–024 | Centered non-divergence rows with same endpoint means on both sides; all prescribed refinements, fixed heat, mode amplification, and independent dense multi-step oracle pass |
| AC025 | `endpointMean` exact branch and separately checked halves; mesh ratios use `(dt/dx)/dx` and `(dt/dx)/2`; safe-mean/finite-underflow tests |
| AC026–027 | `detail/CheckedArithmetic.h` plus separate row products, displayed left-to-right RHS sums and left-before-right boundary corrections; all sealed overflow/order tests |
| AC028 | `PDE.h`, inline definitions; sealed independent umbrella TUs compile/link/run with the normal target |
| AC029 | All solve state/scratch is local; sealed concurrent pure-callback solves produce independent bit-identical arrays |
| AC030 | AS-09 checks all 10 source and installed headers independently/twice; installed consumer also compiles 10 distinct installed-header probes in Debug and Release |
| AC031 | Exact diff preserves module root, static core/provider/presets/dependencies/bindings; domain has standard-library includes only |
| AC032 | Recursive module include directory in `pyproject.toml`; actual sdist compares 48 required native/test/consumer/script inputs byte-for-byte, including all nested headers and sealed support headers; wheel rebuilt from unpacked sdist and imports from a clean venv |
| AC033 | `tests/vcpkg-consumer/main.cpp` asserts one-row outputs for theta .5/.75/1 and fixed heat norms; imported installed target only; relocated/source-hidden Debug and Release; explicit source absence/restoration checks |
| AC034 | Chronological gates and full Linux canonical run; Windows/exact GitHub candidate CI remains dispatcher's next evidence obligation |
| AC035 | Static allocation/commit inspection described above plus transactional tests. Standard resource errors propagate; internal allocation-failure injection remains unautomated |
| AC036 | Volatile-double operation helpers, exact mean/ratio/row/Thomas order and immediate pivot policy; sealed negative cases, diagnostic regression, extra contraction-capable optimization run |
| AC037 | Bidirectional scope inspection: only uniform forward-time 1D Dirichlet theta stepping and Thomas; no fallback, clipping, regularization, extra sign/dominance limits, trajectory, interpolation, cache, configuration mutation, IC/BC mismatch rejection or Python solver API |

## Numerical results

GCC 13.3.0, C++17, Release (`NDEBUG`), binary64 digits 53. Full logs record domain, nx, nt, dx, dt, theta and all errors for all levels. Finest results and adjacent coarse-to-fine rates below come from the canonical run on domain-implementation commit `87a32549308a6b590c498b51bcbfa444a642c6ee`; the follow-up changes only validation assertions.

| Fixture / theta | Finest E_inf | Finest E_2 | p_inf pairs | p_2 pairs |
| --- | ---: | ---: | --- | --- |
| Spatial all terms / .5 | 8.636884e-5 | 6.078789e-5 | 1.998002, 1.999996 | 2.000181, 2.000045 |
| CN temporal / .5 | 6.450064e-8 | 4.733654e-8 | 2.000251, 2.000063 | 2.000243, 2.000061 |
| Temporal / .75 | 3.189567e-5 | 2.266160e-5 | .993220, .996427 | .993259, .996447 |
| Temporal / 1 | 6.346571e-5 | 4.509250e-5 | .978340, .989018 | .978422, .989060 |
| Fixed heat / .5 | 3.022465e-5 | 2.137206e-5 | — | — |

High-mode max absolute discrepancies are `5.8648e-15`, `1.9533e-15`, `1.6588e-17` at theta .5/.75/1. All endpoints match their callback data exactly. The mixed dense oracle passes for .5, `nextafter(.5,1)`, .75 and 1. Extra `-O3 -march=native -ffp-contract=fast` compilation/running of all eight Thomas/arithmetic tests passes (`contraction-probes.log`), substantiating separate operation checks when contraction instructions are available.

## Completed validation and boundaries

Canonical `python scripts/bootstrap.py` and `python scripts/validate.py --wheel` passed. The final full code validation is bound to `49ddf0c800a2958a5c76fedf9ebbc696ee7c7082` in `canonical-hidden-proof.log`: 34 GTests (29 sealed, two baseline hello, three implementation regressions), all three retained Python tests, native cp312 Linux wheel rebuilt from the unpacked sdist, isolated clean-wheel import, and rebuilt installed package/consumer in Release and Debug. The consumer asserts rational one-row outputs and fixed heat norms, and compiles all ten actual installed headers separately/twice. Imported include/binary/config paths stay inside the relocated prefix, with Python/bindings disabled for the port.

`final-package-headers.log` records the actual 48-input sdist byte comparison, AS-09 on both source and installed roots (ten headers each), and unchanged authority/sealed hashes. Artifacts from this run: sdist `pde_software_replication-0.1.0.tar.gz`, SHA256 `755d482d7766796a3bf32ee6776b31e4e8df46b1e8dedc5db16ec3c684a354cb`; wheel `pde_software_replication-0.1.0-cp312-cp312-linux_x86_64.whl`, SHA256 `c0099debba5e531a6361cddbe272e7fc1fb9454d849ca265142d96f2ca882dbb`.

Diagnostic samples in `diagnostic-samples.log` include `missing initial callback`, `callback diffusion requires finite positive value at t=1.000000, x=1.000000`, `scheme row assembly -2ar at step 0, row/index 1`, and `Thomas pivot zero row/pivot at step 0, row/index 1`.

A supplemental `cmake --install` at 17:00:47 overlapped the first canonical consumer's intentional source-header hiding and failed immediately because source `cpp/include` was absent; its partial writes were confined to an external scratch prefix. An untracked byte-identical `cpp/include.consumer-hidden` duplicate was later observed after completed consumer commands, including a sequential rerun without overlap. Its creator is not established, so the overlap is not claimed as its cause. Duplicate snapshots were preserved outside the repository. The follow-up validation assertions establish that source headers stayed absent after configure/build/test in both configurations, and source was restored with the hidden path absent before the process exited. The full follow-up canonical run passed these assertions; thus the observed between-tool duplicate did not compromise the in-process installed-consumer proof. Artifact inventory was rerun after cleanup. Failed supplemental invocations and the initial/sequential/follow-up logs are all retained; no sealed file was changed.

Internal allocator exhaustion was not forced. Static guarantees and representative tests do not exhaust binary64 state space or establish accuracy for arbitrary ill-conditioned/nonsmooth configurations. Windows execution, exact transported GitHub CI, independent review/remediation and fresh final verification remain outstanding. No GitHub mutation or merge was performed by the Implementer.
