#ifndef PDE_SOFTWARE_REPLICATION_CORE_TRIDIAGONAL_H
#define PDE_SOFTWARE_REPLICATION_CORE_TRIDIAGONAL_H
#include "Array.h"
#include "detail/CheckedArithmetic.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>

namespace PDE {
class TridiagonalSolver {
public:
    static void solve(Array1D const& lower, Array1D const& diagonal,
                      Array1D const& upper, Array1D& rhs) {
        if (&rhs == &lower || &rhs == &diagonal || &rhs == &upper)
            throw std::runtime_error("Thomas rhs aliases a coefficient array");
        const std::size_t n = diagonal.size();
        if (n == 0 || rhs.size() != n || lower.size() != n - 1 || upper.size() != n - 1)
            throw std::runtime_error("Thomas shape requires n>=1, rhs n and off-diagonals n-1");
        validateInput(lower, "lower");
        validateInput(diagonal, "diagonal");
        validateInput(upper, "upper");
        validateInput(rhs, "rhs");

        // All writes, including the final back substitution, are private until
        // the complete solution has passed its finite checks.
        Array1D pivots(diagonal), reduced(rhs), solution(n);
        checkPivot(pivots[0], 0, lower, diagonal, upper);
        for (std::size_t j = 1; j < n; ++j) {
            const detail::ArithmeticContext context{"Thomas elimination", j};
            const double w = detail::divide(lower[j - 1], pivots[j - 1], "multiplier division", context);
            const double pivotProduct = detail::multiply(w, upper[j - 1], "pivot product", context);
            pivots[j] = detail::subtract(diagonal[j], pivotProduct, "pivot subtraction", context);
            checkPivot(pivots[j], j, lower, diagonal, upper);
            const double rhsProduct = detail::multiply(w, reduced[j - 1], "rhs product", context);
            reduced[j] = detail::subtract(rhs[j], rhsProduct, "rhs subtraction", context);
        }
        solution[n - 1] = detail::divide(reduced[n - 1], pivots[n - 1], "solution division",
                                         {"Thomas substitution", n - 1});
        for (std::size_t j = n - 1; j > 0; --j) {
            const std::size_t row = j - 1;
            const detail::ArithmeticContext context{"Thomas substitution", row};
            const double product = detail::multiply(upper[row], solution[j], "upper product", context);
            const double numerator = detail::subtract(reduced[row], product, "numerator subtraction", context);
            solution[row] = detail::divide(numerator, pivots[row], "solution division", context);
        }
        rhs.swap(solution);
    }
private:
    static void validateInput(Array1D const& input, char const* role) {
        for (std::size_t j = 0; j < input.size(); ++j)
            (void)detail::finite(input[j], role, {"Thomas nonfinite input", j});
    }
    static void checkPivot(double pivot, std::size_t j, Array1D const& lower,
                           Array1D const& diagonal, Array1D const& upper) {
        double rho = std::abs(diagonal[j]);
        if (j > 0) rho = std::max(rho, std::abs(lower[j - 1]));
        if (j + 1 < diagonal.size()) rho = std::max(rho, std::abs(upper[j]));
        const detail::ArithmeticContext context{"Thomas pivot", j};
        (void)detail::finite(pivot, "nonfinite pivot", context);
        if (rho == 0 || pivot == 0) detail::arithmeticFailure("zero row/pivot", context);
        const double magnitude = std::abs(pivot);
        if (magnitude >= rho) return;
        const double ratio = detail::divide(magnitude, rho, "relative pivot division", context);
        constexpr double tau = 64 * std::numeric_limits<double>::epsilon();
        if (!(ratio > tau)) detail::arithmeticFailure("unusable relative pivot", context);
    }
};
} // namespace PDE
#endif
