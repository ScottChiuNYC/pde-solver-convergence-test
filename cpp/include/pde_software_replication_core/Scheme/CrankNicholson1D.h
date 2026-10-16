#ifndef PDE_SOFTWARE_REPLICATION_CORE_SCHEME_CRANK_NICHOLSON_1D_H
#define PDE_SOFTWARE_REPLICATION_CORE_SCHEME_CRANK_NICHOLSON_1D_H
#include "../Array.h"
#include "../Equation.h"
#include "../Grid.h"
#include "../Tridiagonal.h"
#include "../detail/CheckedArithmetic.h"
#include <limits>
#include <string>
#include <cmath>
#include <stdexcept>

namespace PDE {
namespace detail {
inline double sample(Func::F1D const& callback, char const* role, double t, double x,
                     bool positive = false) {
    const double value = callback(t, x); // User exceptions deliberately escape unchanged.
    if (!std::isfinite(value) || (positive && !(value > 0)))
        throw std::runtime_error(std::string("callback ") + role +
            (positive ? " requires finite positive value" : " requires finite value") +
            " at t=" + std::to_string(t) + ", x=" + std::to_string(x));
    return value;
}
inline double endpointMean(double p, double q, ArithmeticContext context) {
    constexpr double halfMax = std::numeric_limits<double>::max() / 2;
    if (std::abs(p) <= halfMax && std::abs(q) <= halfMax) {
        const double sum = add(p, q, "endpoint sum", context);
        return divide(sum, 2, "endpoint mean", context);
    }
    const double halfP = divide(p, 2, "first half endpoint", context);
    const double halfQ = divide(q, 2, "second half endpoint", context);
    return add(halfP, halfQ, "endpoint half sum", context);
}
inline void validateConfiguration(PDE1D const& pde) {
    for (std::size_t side = 0; side < 2; ++side)
        if (pde.boundaryConfig.types[side] != BoundaryType::Dirichlet)
            throw std::runtime_error("unsupported boundary type on side " + std::to_string(side));
    if (!pde.diffusion) throw std::runtime_error("missing diffusion callback");
    if (!pde.advection) throw std::runtime_error("missing advection callback");
    if (!pde.reaction) throw std::runtime_error("missing reaction callback");
    if (!pde.source) throw std::runtime_error("missing source callback");
    if (!pde.initial) throw std::runtime_error("missing initial callback");
    if (!pde.boundaryL) throw std::runtime_error("missing boundaryL callback");
    if (!pde.boundaryR) throw std::runtime_error("missing boundaryR callback");
}
} // namespace detail
namespace Scheme {
class CrankNicholson1D {
public:
    CrankNicholson1D(Grid1D const& grid, PDE1D const& pde) : m_grid(grid), m_pde(pde) {}
    CrankNicholson1D(CrankNicholson1D const&) = default;
    CrankNicholson1D(CrankNicholson1D&&) = default;
    CrankNicholson1D& operator=(CrankNicholson1D const&) = delete;
    CrankNicholson1D& operator=(CrankNicholson1D&&) = delete;
    void setTheta(double theta) {
        if (!std::isfinite(theta) || theta < 0.5 || theta > 1)
            throw std::runtime_error("scheme theta must be finite and in [0.5, 1]");
        m_theta = theta;
    }
    double theta() const { return m_theta; }
    Array1D solve() const {
        // Mutable PDE fields are checked on every solve, before any scratch
        // storage or user callback. Construction retains references only.
        detail::validateConfiguration(m_pde);
        const std::size_t nx = m_grid.nx(), m = nx - 2;
        const detail::ArithmeticContext meshContext{"scheme mesh ratios", 0};
        const double dtOverDx = detail::divide(m_grid.dt(), m_grid.dx(), "dt/dx", meshContext);
        const double r = detail::divide(dtOverDx, m_grid.dx(), "r=(dt/dx)/dx", meshContext);
        const double s = detail::divide(dtOverDx, 2, "s=(dt/dx)/2", meshContext);
        const double oldWeight = detail::subtract(1, m_theta, "old theta weight", meshContext);
        Array1D oldState(nx), newState(nx), lower(m - 1), diagonal(m), upper(m - 1), rhs(m);
        const auto& domain = m_grid.domain();
        const double t0 = m_grid.t(0);
        for (std::size_t i = 1; i < nx - 1; ++i)
            oldState[i] = detail::sample(m_pde.initial, "initial", t0, m_grid.x(i));
        oldState[0] = detail::sample(m_pde.boundaryL, "boundaryL", t0, domain.xL);
        oldState[nx - 1] = detail::sample(m_pde.boundaryR, "boundaryR", t0, domain.xR);

        for (std::size_t n = 0; n < m_grid.nt(); ++n) {
            const double tn = m_grid.t(n), nextT = m_grid.t(n + 1);
            const double nextLeft = detail::sample(m_pde.boundaryL, "boundaryL", nextT, domain.xL);
            const double nextRight = detail::sample(m_pde.boundaryR, "boundaryR", nextT, domain.xR);
            for (std::size_t j = 0; j < m; ++j) {
                const std::size_t i = j + 1;
                const double x = m_grid.x(i);
                const detail::ArithmeticContext context{"scheme row assembly", i, n};
                const double aOld = detail::sample(m_pde.diffusion, "diffusion", tn, x, true);
                const double aNew = detail::sample(m_pde.diffusion, "diffusion", nextT, x, true);
                const double bOld = detail::sample(m_pde.advection, "advection", tn, x);
                const double bNew = detail::sample(m_pde.advection, "advection", nextT, x);
                const double cOld = detail::sample(m_pde.reaction, "reaction", tn, x);
                const double cNew = detail::sample(m_pde.reaction, "reaction", nextT, x);
                const double a = detail::endpointMean(aOld, aNew, context);
                const double b = detail::endpointMean(bOld, bNew, context);
                const double c = detail::endpointMean(cOld, cNew, context);

                const double ar = detail::multiply(a, r, "ar", context);
                const double bs = detail::multiply(b, s, "bs", context);
                const double cdt = detail::multiply(c, m_grid.dt(), "cdt", context);
                const double qL = detail::subtract(ar, bs, "qL", context);
                const double minusTwoAr = detail::multiply(-2, ar, "-2ar", context);
                const double qD = detail::add(minusTwoAr, cdt, "qD", context);
                const double qR = detail::add(ar, bs, "qR", context);
                const double aL = detail::multiply(-m_theta, qL, "implicit left", context);
                const double thetaQD = detail::multiply(m_theta, qD, "theta qD", context);
                const double aD = detail::subtract(1, thetaQD, "implicit diagonal", context);
                const double aR = detail::multiply(-m_theta, qR, "implicit right", context);

                // Evaluate every product before the displayed, left-to-right sum.
                const double oldLeftWeight = detail::multiply(oldWeight, qL, "old left weight", context);
                const double oldLeft = detail::multiply(oldLeftWeight, oldState[i - 1], "old left product", context);
                const double oldDiagonalDelta = detail::multiply(oldWeight, qD, "old diagonal delta", context);
                const double oldDiagonalWeight = detail::add(1, oldDiagonalDelta, "old diagonal weight", context);
                const double oldCenter = detail::multiply(oldDiagonalWeight, oldState[i], "old center product", context);
                const double oldRightWeight = detail::multiply(oldWeight, qR, "old right weight", context);
                const double oldRight = detail::multiply(oldRightWeight, oldState[i + 1], "old right product", context);
                double oldSource = 0;
                if (m_theta < 1) {
                    const double fOld = detail::sample(m_pde.source, "source", tn, x);
                    oldSource = detail::multiply(oldWeight, fOld, "old source product", context);
                }
                const double fNew = detail::sample(m_pde.source, "source", nextT, x);
                const double newSource = detail::multiply(m_theta, fNew, "new source product", context);
                const double weightedSource = detail::add(oldSource, newSource, "weighted source sum", context);
                const double sourceTerm = detail::multiply(m_grid.dt(), weightedSource, "source dt product", context);
                double value = detail::add(oldLeft, oldCenter, "rhs left plus center", context);
                value = detail::add(value, oldRight, "rhs plus right", context);
                value = detail::add(value, sourceTerm, "rhs plus source", context);
                if (j == 0) {
                    const double correction = detail::multiply(aL, nextLeft, "left boundary product", context);
                    value = detail::subtract(value, correction, "left boundary correction", context);
                } else lower[j - 1] = aL;
                if (j == m - 1) {
                    const double correction = detail::multiply(aR, nextRight, "right boundary product", context);
                    value = detail::subtract(value, correction, "right boundary correction", context);
                } else upper[j] = aR;
                diagonal[j] = aD;
                rhs[j] = value;
            }
            TridiagonalSolver::solve(lower, diagonal, upper, rhs);
            newState[0] = nextLeft;
            newState[nx - 1] = nextRight;
            for (std::size_t j = 0; j < m; ++j) newState[j + 1] = rhs[j];
            oldState.swap(newState);
        }
        return oldState;
    }
private:
    Grid1D const& m_grid;
    PDE1D const& m_pde;
    double m_theta = 0.5;
};
} // namespace Scheme
inline Scheme::CrankNicholson1D makeSolver1D(Grid1D const& grid, PDE1D const& pde) {
    return Scheme::CrankNicholson1D(grid, pde);
}
} // namespace PDE
#endif
