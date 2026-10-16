#ifndef PDE_SOFTWARE_REPLICATION_CORE_GRID_H
#define PDE_SOFTWARE_REPLICATION_CORE_GRID_H
#include "Domain.h"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace PDE {
class Grid1D {
public:
    Grid1D(Domain1D const& domain, std::size_t nx, std::size_t nt)
        : m_domain(domain), m_nx(nx), m_nt(nt), m_dx(0), m_dt(0) {
        if (nx < 3) throw std::runtime_error("grid nx must be at least 3");
        if (nt < 1) throw std::runtime_error("grid nt must be at least 1");
        if (nx > std::vector<double>().max_size()) throw std::runtime_error("grid nx exceeds vector max_size");
        detail::validateEndpoints(m_domain);
        const double xSpan = detail::positiveSpan(m_domain.xR, m_domain.xL, "x span");
        const double tSpan = detail::positiveSpan(m_domain.t1, m_domain.t0, "t span");
        const double dx = detail::divide(xSpan, static_cast<double>(nx - 1), "dx division", {"grid", 0});
        if (!(dx > 0)) throw std::runtime_error("grid dx must be positive");
        const double dt = detail::divide(tSpan, static_cast<double>(nt), "dt division", {"grid", 0});
        if (!(dt > 0)) throw std::runtime_error("grid dt must be positive");
        m_dx = dx;
        m_dt = dt;
    }
    std::size_t nx() const { return m_nx; }
    std::size_t nt() const { return m_nt; }
    double dx() const { return m_dx; }
    double dt() const { return m_dt; }
    double x(std::size_t i) const {
        if (i >= m_nx) throw std::runtime_error("grid x index out of bounds: " + std::to_string(i));
        if (i == 0) return m_domain.xL;
        if (i == m_nx - 1) return m_domain.xR;
        const detail::ArithmeticContext context{"grid x coordinate", i};
        const double offset = detail::multiply(static_cast<double>(i), m_dx, "offset", context);
        return detail::add(m_domain.xL, offset, "sum", context);
    }
    double t(std::size_t k) const {
        if (k > m_nt) throw std::runtime_error("grid t index out of bounds: " + std::to_string(k));
        if (k == 0) return m_domain.t0;
        if (k == m_nt) return m_domain.t1;
        const detail::ArithmeticContext context{"grid t coordinate", k};
        const double offset = detail::multiply(static_cast<double>(k), m_dt, "offset", context);
        return detail::add(m_domain.t0, offset, "sum", context);
    }
    Domain1D const& domain() const { return m_domain; }
private:
    Domain1D m_domain;
    std::size_t m_nx, m_nt;
    double m_dx, m_dt;
};
} // namespace PDE
#endif
