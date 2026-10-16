#ifndef PDE_SOFTWARE_REPLICATION_CORE_TYPES_H
#define PDE_SOFTWARE_REPLICATION_CORE_TYPES_H
#include <array>
#include <cstddef>
#include <functional>

namespace PDE {
namespace Func { using F1D = std::function<double(double, double)>; }
enum class BoundaryType { Dirichlet, Neumann, Gamma };
struct BoundaryConfig1D {
    static constexpr std::size_t Left = 0, Right = 1;
    std::array<BoundaryType, 2> types{BoundaryType::Dirichlet, BoundaryType::Dirichlet};
};
} // namespace PDE
#endif
