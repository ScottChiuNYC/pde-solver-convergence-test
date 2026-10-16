#ifndef PDE_SOFTWARE_REPLICATION_CORE_DOMAIN_H
#define PDE_SOFTWARE_REPLICATION_CORE_DOMAIN_H
#include "detail/CheckedArithmetic.h"
#include <cmath>
#include <stdexcept>
#include <string>

namespace PDE {
struct Domain1D {
    double xL, xR, t0, t1;
    inline Domain1D(double xL, double xR, double t0, double t1);
};
namespace detail {
inline void validateEndpoints(Domain1D const& d) {
    if (!std::isfinite(d.xL)) throw std::runtime_error("domain xL must be finite");
    if (!std::isfinite(d.xR)) throw std::runtime_error("domain xR must be finite");
    if (!std::isfinite(d.t0)) throw std::runtime_error("domain t0 must be finite");
    if (!std::isfinite(d.t1)) throw std::runtime_error("domain t1 must be finite");
    if (!(d.xR > d.xL)) throw std::runtime_error("domain requires xR > xL");
    if (!(d.t1 > d.t0)) throw std::runtime_error("domain requires t1 > t0");
}
inline double positiveSpan(double right, double left, char const* field) {
    const double span = subtract(right, left, field, {"domain", 0});
    if (!(span > 0)) throw std::runtime_error(std::string("domain ") + field + " must be positive");
    return span;
}
} // namespace detail
inline Domain1D::Domain1D(double left, double right, double start, double finish)
    : xL(left), xR(right), t0(start), t1(finish) {
    detail::validateEndpoints(*this);
    (void)detail::positiveSpan(xR, xL, "x span");
    (void)detail::positiveSpan(t1, t0, "t span");
}
} // namespace PDE
#endif
