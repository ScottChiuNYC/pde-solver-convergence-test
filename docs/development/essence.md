# 1D Parabolic PDE Solver Essence

<!-- ICC-FREEZE-PROVENANCE:BEGIN -->
Status: **FROZEN / IMPLEMENTATION_READY**

- Audited authority commit: `23c0e26209bfd3f992f556dbc6ad97822ca2730b`
- Audited Essence blob: `4c88b6656bba9adc71907cadaf4de1da8d64f3a9`
- Latest reconciled PASS: [formal round 2](https://github.com/ScottChiuNYC/pde-solver-replication/issues/13)
- Unresolved owner decisions: **0**
<!-- ICC-FREEZE-PROVENANCE:END -->

## 1. Scope and Project Boundary

Provide a header-only C++17 library for

\[
u_t=a(t,x)u_{xx}+b(t,x)u_x+c(t,x)u+f(t,x),\qquad x\in[x_L,x_R],\ t\in[t_0,t_1].
\]

The public name `CrankNicholson1D` denotes the theta family specified below: default Crank–Nicolson (`theta=0.5`), with every finite `theta` in `[0.5,1]` supported. Only one spatial dimension, uniform grids, forward time, and Dirichlet boundaries are supported. Neumann and Gamma are enum values for recognition and rejection, not implemented methods. There is no Python solver API, adaptive grid, interpolation, trajectory output, or alternate linear solver in this scope.

This file is the entire normative domain contract. `PROJECT_SKELETON_CONTRACT.md` (version 1.1, in the same directory) is the Project Boundary and governs repository locations, build targets, packaging, bindings, tests, examples, CI, and clean C++ consumers. That contract governs infrastructure; this file governs domain behavior. Neither overrides requirements owned by the other.

Logical public units are `Types.h`, `Domain.h`, `Grid.h`, `Array.h`, `Equation.h`, `Tridiagonal.h`, the `Scheme::CrankNicholson1D` unit, and the umbrella `PDE.h`. The implementation maps them through the Skeleton's module public include root; the consumer umbrella spelling is exactly:

```cpp
#include <pde_software_replication_core/PDE.h>
```

The umbrella exposes every declaration below. Individual headers must be self-contained, guarded, and included by the umbrella. All domain definitions are inline/header-only and usable in multiple translation units through the existing core package target. Any logical header hierarchy must be covered by the Skeleton's install and source-distribution rules. Do not introduce a second public include root. Standard-library dependencies only; no project-defined templates or external numerical library are required. Private scratch storage and helper decomposition are implementation choices subject to this contract.

## 2. Public C++ surface

These are the complete required public declarations; method bodies implement the semantics in Sections 3–6. All names are in the namespaces shown. No exception specifications are required.

```cpp
#include <array>
#include <cstddef>
#include <functional>
#include <vector>

namespace PDE {
namespace Func { using F1D = std::function<double(double, double)>; }

enum class BoundaryType { Dirichlet, Neumann, Gamma };
struct BoundaryConfig1D {
    static constexpr std::size_t Left = 0, Right = 1;
    std::array<BoundaryType, 2> types{
        BoundaryType::Dirichlet, BoundaryType::Dirichlet};
};

struct Domain1D {
    double xL, xR, t0, t1;
    Domain1D(double xL, double xR, double t0, double t1);
};

class Grid1D {
public:
    Grid1D(Domain1D const& domain, std::size_t nx, std::size_t nt);
    std::size_t nx() const;
    std::size_t nt() const;
    double dx() const;
    double dt() const;
    double x(std::size_t i) const;
    double t(std::size_t k) const;
    Domain1D const& domain() const;
private:
    Domain1D m_domain;
    std::size_t m_nx, m_nt;
    double m_dx, m_dt;
};

class Array1D {
public:
    using iterator = std::vector<double>::iterator;
    using const_iterator = std::vector<double>::const_iterator;
    Array1D();
    explicit Array1D(std::size_t size, double value = 0.0);
    Array1D(Array1D const&);
    Array1D(Array1D&&);
    Array1D& operator=(Array1D const&);
    Array1D& operator=(Array1D&&);
    std::size_t size() const;
    double& operator[](std::size_t i);
    double const& operator[](std::size_t i) const;
    double& at(std::size_t i);
    double const& at(std::size_t i) const;
    void fill(double value);
    void swap(Array1D& other);
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    double* data();
    double const* data() const;
private:
    std::vector<double> m_data;
};

struct PDE1D {
    PDE1D();
    Func::F1D diffusion, advection, source, reaction;
    Func::F1D initial, boundaryL, boundaryR;
    BoundaryConfig1D boundaryConfig;
};

class TridiagonalSolver {
public:
    static void solve(Array1D const& lower, Array1D const& diagonal,
                      Array1D const& upper, Array1D& rhs);
};

namespace Scheme {
class CrankNicholson1D {
public:
    CrankNicholson1D(Grid1D const& grid, PDE1D const& pde);
    CrankNicholson1D(CrankNicholson1D const&) = default;
    CrankNicholson1D(CrankNicholson1D&&) = default;
    CrankNicholson1D& operator=(CrankNicholson1D const&) = delete;
    CrankNicholson1D& operator=(CrankNicholson1D&&) = delete;
    void setTheta(double theta);
    double theta() const;
    Array1D solve() const;
private:
    Grid1D const& m_grid;
    PDE1D const& m_pde;
    double m_theta = 0.5;
};
}
inline Scheme::CrankNicholson1D makeSolver1D(Grid1D const& grid,
                                           PDE1D const& pde);
}
```

`Domain1D`, `Grid1D`, `PDE1D`, and `BoundaryConfig1D` have ordinary value copy/move/assignment semantics. `Grid1D` owns a copy of its domain; `domain()` returns a reference valid until the grid is destroyed or assigned. Domain fields are public; a grid always validates its own copied values, including a domain changed after construction.

`Array1D` owns contiguous `double` storage in increasing index order. The default is empty; the size constructor creates exactly `size` elements initialized to `value`, including size zero. Nonfinite stored values are permitted in the general container. Copying is deep; moving transfers storage and leaves the source empty (self-move leaves the object unchanged). Copy assignment has the strong exception guarantee. `fill` changes all values without changing size. Both `operator[]` and `at` check `i<size()` and otherwise throw `std::runtime_error`. Iterators span `[begin(),end())`; `data()` points to the first element and is `nullptr` when empty. Caller iterator/pointer arithmetic must stay within the storage bounds. Assignment, move, and destruction may invalidate access to the affected storage; swap transfers storage ownership and existing element references follow that storage. Fill and element assignment do not invalidate access. There is no padded or external-storage view.

The scheme and factory retain references, never copies of the grid or PDE. Both referenced objects and objects referenced by their callbacks must outlive every use of the scheme, including copied/moved schemes; do not pass temporary configurations. Scheme copying/moving preserves the references and theta. Construction/factory calls do not evaluate callbacks or validate the PDE's mutable fields. Changes to configurations between solves are observed by the next solve. Do not mutate them during a solve. `setTheta` accepts exactly finite `[0.5,1]`; invalid input throws `std::runtime_error` and leaves theta unchanged. `theta()` returns the current value. `solve()` has no persistent trajectory, factorization cache, or mutation of its configurations or theta; success returns an independent owning array of `nx` final-time values. Failure returns no partial array. Concurrent solves require unchanged configurations and callbacks safe for concurrent use; concurrent setters are not supported.

## 3. Grid, data, and errors

`nx` is the number of spatial nodes including both endpoints; require `nx>=3`. `nt>=1` is the number of time intervals, with `nt+1` mathematical time nodes. Domain endpoints must be finite and ordered `xR>xL`, `t1>t0`; both spans, and computed `dx=(xR-xL)/(nx-1)`, `dt=(t1-t0)/nt`, must be finite and positive. `Domain1D` checks endpoints and spans; `Grid1D` rechecks them and cardinalities before division or allocation. Reject `nx` exceeding `std::vector<double>().max_size()` before allocation. First check cardinalities and endpoints, then checked spans and spacings; invalid arithmetic results must not be retained or used for allocation/stepping.

Spatial indices are `0..nx-1`, time indices `0..nt`. `x(0)=xL`, `x(nx-1)=xR`, `t(0)=t0`, `t(nt)=t1` exactly; other coordinates are `xL+double(i)*dx` and `t0+double(k)*dt`. Invalid indices or a nonfinite computed coordinate throw `std::runtime_error`. Grid construction stores no sampled PDE data.

Callbacks have arguments `(t,x)` in the caller's consistent physical units. The solver performs no unit conversion. Default `PDE1D` has nonempty constant callbacks `diffusion=1`, `advection=reaction=source=0`, two Dirichlet types, and empty `initial`, `boundaryL`, `boundaryR`. All seven callbacks must be nonempty before every solve; empty coefficient callbacks are not reset to defaults. Before allocating solve scratch or invoking any callback, reject unsupported boundary types (anything other than Dirichlet on either side) and any missing callback.

The required sample set is:

- `initial(t0,x(i))` at interior indices `i=1..nx-2` only;
- `boundaryL(t(k),xL)` and `boundaryR(t(k),xR)` for `k=0..nt`;
- `diffusion`, `advection`, and `reaction` at every `(t(k),x(i))` with `k=0..nt` and interior `i`;
- `source` at the same interior nodes for `k=0..nt` when theta is less than 1, and for `k=1..nt` when theta is 1. The zero-weight old source at theta 1 is not evaluated.

Every sampled result must be finite, and every sampled diffusion must be strictly positive. There are no coefficient sign restrictions beyond positive diffusion: finite general advection, reaction, and source are admitted for a checked solve. Values outside this sample set are irrelevant and must not be required. Callbacks must give repeatable values for the same arguments and must not mutate configurations; evaluation order/count and repeated evaluation of required samples are not an API guarantee. Exceptions thrown by user callbacks propagate unchanged, without wrapping, substitution, or retries. The solver does not undo user callback side effects.

Use double precision with normal finite-value checks, round-to-nearest arithmetic and gradual underflow on the supported binary64 platforms. Underflow to a finite value is allowed; NaN/infinity in sampled data or generated coordinates, averages, row assembly, elimination, or substitution causes `std::runtime_error`. Do not silently clip values, change mesh/theta, regularize a matrix, pivot, or fall back to another method. Implementation-generated validation/numerical exceptions identify the failing category and relevant callback/coordinate, grid field, step/row, or pivot index; exact message wording is not prescribed. Bounds/configuration/shape/pivot violations use `std::runtime_error`. Standard resource failures such as `std::bad_alloc` propagate unchanged. No success or accuracy guarantee is made for every admitted coefficient/mesh combination.

## 4. Initialization and theta stepping

Let `x_i=grid.x(i)`, `t_n=grid.t(n)`, and `m=nx-2`. Initialize only interior values from `initial(t0,x_i)`. Set `U_0^0=boundaryL(t0,xL)` and `U_{nx-1}^0=boundaryR(t0,xR)`. Dirichlet data take precedence at both initial corners: do not evaluate the initial callback at endpoints, compare IC and BC, apply a compatibility tolerance, or reject a corner mismatch.

For each step `n=0..nt-1` and each interior `i`, use the arithmetic endpoint means of the coefficients at the same spatial node:

\[
\bar a_i=\tfrac12(a(t_n,x_i)+a(t_{n+1},x_i)),\quad
\bar b_i=\tfrac12(b(t_n,x_i)+b(t_{n+1},x_i)),\quad
\bar c_i=\tfrac12(c(t_n,x_i)+c(t_{n+1},x_i)).
\]

For finite operands `p,q`, evaluate a mean as `(p+q)/2` when both magnitudes are at most `numeric_limits<double>::max()/2`, otherwise as `p/2+q/2`. This avoids a spurious overflow in averaging finite same-sign values. Form `r=(dt/dx)/dx` and `s=(dt/dx)/2`, the floating-point evaluation of `dt/dx^2` and `dt/(2*dx)`. Define

\[
q_L=\bar a_i r-\bar b_i s,\quad
q_D=-2\bar a_i r+\bar c_i dt,\quad
q_R=\bar a_i r+\bar b_i s.
\]

These implement centered `u_xx=(U_{i-1}-2U_i+U_{i+1})/dx^2` and `u_x=(U_{i+1}-U_{i-1})/(2dx)`, with coefficients multiplying these derivatives (not a divergence-form discretization). The same averaged `a,b,c` are used on both time sides for every supported theta. The full row is

\[
A_L=-\theta q_L,\quad A_D=1-\theta q_D,\quad A_R=-\theta q_R,
\]
\[
B_i=(1-\theta)q_L U_{i-1}^n+
       (1+(1-\theta)q_D)U_i^n+
       (1-\theta)q_R U_{i+1}^n+
       dt\big((1-\theta)f(t_n,x_i)+\theta f(t_{n+1},x_i)\big).
\]

For row evaluation compute `ar=a_bar*r`, `bs=b_bar*s`, and `cdt=c_bar*dt` first; use `qL=ar-bs`, `qD=(-2*ar)+cdt`, `qR=ar+bs`. Evaluate the three old-state products and the source term separately, then add them in the displayed order. At theta 1 the old source product is zero without callback evaluation. Apply the left boundary correction before the right. Products, divisions, and sums/subtractions are separate double results, each checked for finiteness; fused contraction or wider intermediates must not bypass these checks.

Thus the explicit right-neighbor term uses `U[i+1]`. Old endpoint values in this expression are Dirichlet data at `t_n`, including `t0`. Before the interior solve, subtract `A_L*boundaryL(t_{n+1},xL)` from the first RHS and `A_R*boundaryR(t_{n+1},xR)` from the last RHS, and omit those two known-boundary matrix entries. Both corrections apply to the same row when `nx=3`. Retain all other interior entries, indexed in increasing `i`, and solve with Section 5. Put the interior solution into the new state and set both endpoints to the corresponding sampled data at `t_{n+1}`. Do exactly `nt` steps; returned endpoints are the callback values at `t1` without transformation.

This is second order in space for smooth solutions. At theta 0.5 it is second order in time for smooth compatible data; other fixed theta values in `(0.5,1]` are generally first order in time. Corner discontinuities carry no smooth-convergence claim. For constant heat diffusion and homogeneous Dirichlet data a sine mode with discrete Laplacian eigenvalue `lambda<=0` has amplification

\[
G=\frac{1+(1-\theta)dt\lambda}{1-\theta dt\lambda},\qquad |G|\le1\quad(\theta\in[0.5,1]).
\]

This property does not assert diagonal dominance, monotonicity, or stability for arbitrary advection/reaction configurations.

## 5. Tridiagonal contract

`TridiagonalSolver::solve(lower,diagonal,upper,rhs)` uses Thomas elimination without pivoting. With `n=diagonal.size()`, require `n>=1`, `rhs.size()==n`, and both off-diagonal sizes `n-1`. For zero-based rows, `lower[j-1]=A[j,j-1]` for `j=1..n-1`, `diagonal[j]=A[j,j]`, and `upper[j]=A[j,j+1]` for `j=0..n-2`. For `n=1` both off-diagonals are empty. Every input element must be finite. The RHS object must not be the same object as any coefficient array; reject aliasing before any mutation. Coefficient arrays may alias each other and are always preserved. On success only RHS changes, becoming the length-`n` solution; on any failure, including allocation or later numerical failure, RHS and coefficient inputs retain their original values. Compute in private scratch and commit only after the whole solution is finite.

For each original input row define

\[
\rho_j=\max\big(|A_{jj}|,\ |A_{j,j-1}|\ (j>0),\ |A_{j,j+1}|\ (j+1<n)\big),
\qquad \tau=64\,\texttt{numeric_limits<double>::epsilon()}.
\]

Only existing matrix entries enter this maximum: no RHS, eliminated boundary entry, global norm, or fixed absolute floor. A row with `rho_j==0` fails. Every reduced pivot `p_j` must be finite, nonzero, and satisfy `abs(p_j)/rho_j > tau`. Equality fails. To evaluate without ratio overflow, accept this comparison when `abs(p_j)>=rho_j`; otherwise divide `abs(p_j)/rho_j` and compare to `tau`. Tiny uniformly scaled nonsingular systems are not rejected merely for having entries below 1.

The prescribed recurrence is `p[0]=diagonal[0]`, `y[0]=rhs[0]`; for `j=1..n-1`, in order:

\[
w_j=\mathrm{lower}[j-1]/p_{j-1},\quad
p_j=\mathrm{diagonal}[j]-w_j\,\mathrm{upper}[j-1],\quad
y_j=\mathrm{rhs}[j]-w_j y_{j-1}.
\]

Apply the row policy immediately after forming each pivot, including `p[0]`, before any subsequent recurrence operation or use as a divisor. Back substitute `x[n-1]=y[n-1]/p[n-1]` and `x[j]=(y[j]-upper[j]*x[j+1])/p[j]` in decreasing `j`. Each division, product, and subtraction in these recurrences must have a finite result, including products before subtraction; do not hide a nonfinite intermediate with fused contraction or algebraic cancellation. Reject the first encountered unusable pivot or nonfinite recurrence result with `std::runtime_error`. This policy checks numerical usability of this algorithm; it is not a condition-number bound or a guarantee that every nonsingular matrix can be solved by Thomas.

## 6. C++17 usage

```cpp
#include <pde_software_replication_core/PDE.h>
#include <cmath>

int main() {
    const double pi = std::acos(-1.0);
    PDE::Domain1D domain(0.0, 1.0, 0.0, 0.1);
    PDE::Grid1D grid(domain, 101, 1000);
    PDE::PDE1D pde;
    pde.initial = [pi](double, double x) { return std::sin(pi*x); };
    pde.boundaryL = [](double, double) { return 0.0; };
    pde.boundaryR = [](double, double) { return 0.0; };
    auto solver = PDE::makeSolver1D(grid, pde);
    PDE::Array1D solution = solver.solve();
    return solution.size() == grid.nx() ? 0 : 1;
}
```

## 7. Objective acceptance

Acceptance belongs in the existing Skeleton test suites, not only an example. All checks below are required on supported platforms. Norms are evaluated at final time over all returned nodes, with `e_i=U_i-u_exact(t1,x_i)`:

\[
E_\infty=\max_i|e_i|,\qquad E_2=\sqrt{dx\sum_{i=0}^{nx-1}e_i^2},\qquad
p=\log_2(E_{\rm coarse}/E_{\rm fine}).
\]

In failure fixtures, `max_double` denotes `numeric_limits<double>::max()` and `denorm_min` denotes `numeric_limits<double>::denorm_min()`. Every norm must be finite. A required rate is checked separately for both norms at both adjacent refinement pairs; zero/nonpositive errors do not count as passing rates. In refinement cases initial and boundary data are the exact solution restricted to the specified nodes; these smooth fixtures are compatible at both initial corners. Set all four coefficient/source callbacks explicitly except in a stated default-data case.

### 7.1 Numerical evidence

Use `pi=acos(-1)` and `x=[0,1]` in the following fixtures.

| Case | Complete data and refinements | Required result |
| --- | --- | --- |
| Spatial convergence with all terms | `a=1+0.2t+0.1x`, `b=0.3+0.1t-0.2x`, `c=-0.4+0.2t+0.1x`; `P=1+x+sin(pi*x)`, `u=exp(t)*P`, `f=exp(t)*((1-c)*P+a*pi*pi*sin(pi*x)-b*(1+pi*cos(pi*x)))`; `t=[0,0.1]`, theta `0.5`, fixed `nt=4096`, `nx=21,41,81` | Both rates in `[1.85,2.15]`; finest `E_inf<=2e-4`, `E_2<=2e-4`. Only space is refined. |
| CN temporal convergence with time-dependent coefficients and boundaries | Same `a,b,c`; `P=1+x+x*x`, `u=exp(t)*P`, `f=exp(t)*((1-c)*P-2*a-b*(1+2*x))`; `t=[0,0.2]`, theta `0.5`, fixed `nx=41`, `nt=20,40,80` | Both rates in `[1.8,2.2]`; finest `E_inf<=2e-6`, `E_2<=2e-6`. Centered spatial derivatives of this polynomial are exact, so no spatial truncation floor contaminates the time refinement. |
| Nondefault theta temporal order | `a=1,b=c=0`, `P=x*(1-x)`, `u=exp(t)*P`, `f=exp(t)*(P+2)`; `t=[0,0.2]`, fixed `nx=41`, `nt=10,20,40`; run separately with theta `0.75` and `1` | Both rates in `[0.85,1.15]` for each theta; finest `E_inf<=1e-4`, `E_2<=7e-5`. Spatial derivatives are exact on the solution. |
| Fixed heat benchmark | `a=1,b=c=f=0`, IC `sin(pi*x)`, exactly zero endpoint callbacks, `t=[0,0.1]`, theta `0.5`, `nx=101,nt=1000`; reference `exp(-pi*pi*0.1)*sin(pi*x)` | `E_inf<=4e-5`, `E_2<=3e-5`. This is additional finite-mesh evidence. |

Also require these direct checks:

- **One interior row, corner precedence, and weighted rows:** `x=[0,2]`, `t=[0,1]`, `nx=3,nt=1`; `a=1+t,b=2*t,c=-2*t,f=2+4*t`, initial callback returns `2` at the interior, `boundaryL=1+2*t`, `boundaryR=5-t`. For theta `0.5`, `0.75`, and `1`, endpoints must be exactly `3,4` and the interior must differ by at most `1e-12` from `(7+12*theta)/(1+4*theta)`. An initial callback that throws at either endpoint must still succeed here. This is a corner-discontinuity behavior check, not a convergence case.
- **Heat-mode stability and formula:** `x=[0,1]`, `t=[0,1]`, `nx=21,nt=1`, `a=1,b=c=f=0`, zero boundaries, initial `sin(19*pi*x)`. With `h=1/20`, `lambda=-4*sin(19*pi/40)^2/(h*h)`, the final vector must agree with `G*sin(19*pi*x)` within `2e-11` at each node and `abs(G)<=1`. Run theta `0.5,0.75,1`; endpoints are exactly zero.
- **Defaults and references:** with `nx=5,nt=2`, `x=[0,1]`, `t=[0,0.1]`, supply only initial/left/right callbacks all returning `1` to the default PDE. Require theta `0.5` and final values within `1e-12` of `1`. Then change those three callbacks to return `2` on the same PDE object; the existing scheme must return values within `1e-12` of `2`. Repeated unchanged solves in one build must produce identical arrays; earlier returned arrays must remain unchanged.

### 7.2 Public helpers and failure evidence

- Construct arrays of sizes zero, one, and three; check initial values, all const/mutable access forms, contiguous data, iterators, fill, deep copy, moved-from emptiness, self-move, and swap. Both checked accessors reject index `size()` without mutation. A copied grid must survive changes/destruction of the source domain; grid endpoints, spacings, and legal last indices must follow Section 3. Invalid grid/array indices throw `std::runtime_error`.
- Reject `nx=0,1,2`, `nt=0`, unordered/equal endpoints, NaN/infinite endpoints, overflowed spans, and spacings rounded to zero before stepping or solve allocation. Explicit zero-spacing examples use a span `numeric_limits<double>::denorm_min()` and divide it by two (`nx=3` for space or `nt=2` for time). Revalidate a domain whose public fields were modified before grid construction.
- A default PDE with missing data must fail before calling any callback. For each of the seven callback fields separately, make it empty while all others are valid and require `std::runtime_error`. For each callback role separately, return NaN, positive infinity, and negative infinity at one required sample and require the same failure class. Return diffusion `0` and `-1` at a required interior sample and require failure. Values/exceptions only at unused initial endpoints or outside the required sample set must not cause failure; explicitly check a throwing source at `t0` is unused when theta is 1.
- For each callback role separately, throw a distinctive user exception at a required sample and verify it escapes with its original type and payload. Both sides must reject Neumann, Gamma, and any other non-Dirichlet enum value before callback invocation. Invalid theta values `0.49`, `1.01`, NaN, and both infinities throw and preserve the previous theta; finite `0.5`, `0.75`, `1`, and `nextafter(0.5,1)` are accepted.
- Thomas success cases: `n=1`, empty off-diagonals, diagonal `2`, RHS `6` gives `3`; and lower/upper both `[1,1]`, diagonal `[4,4,4]`, RHS `[6,12,14]` gives `[1,2,3]`. Require absolute solution error at most `1e-12` and exact preservation of coefficient inputs. Repeat both after multiplying every matrix entry and RHS by `2^-500` and by `2^500`; they must succeed with the same solution tolerance.
- Thomas failures: `n=0`, each wrong vector length, RHS/coefficient object aliasing, nonfinite entries in each input role, a zero row, a zero first pivot, and an elimination-created zero pivot all fail without changing any input. The nonsingular matrix with diagonal `[0,1]` and lower/upper `[1]` must fail rather than pivot. For diagonal `[1,1+delta]`, lower/upper `[1]`, RHS `[2,2+delta]`, require pivot rejection at `delta=2^-48` and at `delta=2^-46`, but success with solution `[1,1]` within `1e-12` at `delta=2^-44`. Repeat these threshold cases with the two uniform scales above. Also require rejection at exact threshold equality: diagonal `[1,1]`, lower `[1]`, upper `[1-tau]`, RHS `[2-tau,2]` has `p[1]=tau` and `rho[1]=1`. Repeat this equality case at both uniform scales. These checks exercise the original-row scale without an absolute floor.
- Generated nonfinite arithmetic must fail transactionally: for Thomas use diagonal `[denorm_min]`, RHS `[max_double]`, empty off-diagonals; for PDE use valid `x=[0,1],t=[0,1],nx=3,nt=1`, diffusion returning `max_double` and otherwise finite heat data, which overflows row generation. A PDE with `x=[0,2],t=[0,1],nx=3,nt=1,theta=0.5,a=1,b=f=0,c=4`, interior IC `1` and zero boundaries produces a zero implicit diagonal with RHS `2` and must throw, not claim dominance or return a value.

### 7.3 Integration evidence and implementation gates

1. Implement the public types, grid/container behavior, and umbrella first; gate on helper/invalid-grid checks and C++17 compilation of Section 6 through the module include prefix.
2. Implement Thomas; gate on all shape, scaling, pivot, preservation, and nonfinite cases in Section 7.2 before wiring PDE steps.
3. Implement callback validation, initialization, and stepping; gate on defaults, reference behavior, boundary/theta/callback failures, one-row exact values, and heat-mode amplification.
4. Gate completion on all numerical refinements and the fixed benchmark plus the Skeleton's normal build/test/package validation. Public clients must compile and run from at least two translation units using the umbrella (no duplicate definitions), and the independent installed/relocated C++ consumer must exercise the solver without source-tree headers. Keep the existing Python tests, native wheel-from-sdist import, and C++ package-consumer validations passing; do not add a Python solver interface to satisfy these checks. Packaging must include every new public header, including any nested logical units.

Record grid/domain, theta, fixture identity, both errors and pairwise rates, compiler/build identity, and source revision in acceptance output so a failed result is reproducible. These gates establish the specified API and numerical evidence for this scope; they do not assert accuracy for arbitrary nonsmooth data or ill-conditioned admitted systems.
