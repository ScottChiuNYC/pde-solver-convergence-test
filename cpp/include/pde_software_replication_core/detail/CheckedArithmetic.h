#ifndef PDE_SOFTWARE_REPLICATION_CORE_DETAIL_CHECKED_ARITHMETIC_H
#define PDE_SOFTWARE_REPLICATION_CORE_DETAIL_CHECKED_ARITHMETIC_H

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace PDE {
namespace detail {
struct ArithmeticContext {
    char const* category;
    std::size_t row;
    std::size_t step = 0;
};

[[noreturn]] inline void arithmeticFailure(char const* operation, ArithmeticContext context) {
    throw std::runtime_error(std::string(context.category) + " " + operation +
        " at step " + std::to_string(context.step) + ", row/index " + std::to_string(context.row));
}

inline double finite(double value, char const* operation, ArithmeticContext context) {
    if (!std::isfinite(value)) arithmeticFailure(operation, context);
    return value;
}

// Volatile stores force a separately rounded binary64 result for every operation.
// In particular a compiler cannot fuse a checked product and later subtraction,
// or retain a wider intermediate that conceals overflow before the finite check.
inline double add(double a, double b, char const* operation, ArithmeticContext context) {
    volatile double result = a + b;
    return finite(result, operation, context);
}
inline double subtract(double a, double b, char const* operation, ArithmeticContext context) {
    volatile double result = a - b;
    return finite(result, operation, context);
}
inline double multiply(double a, double b, char const* operation, ArithmeticContext context) {
    volatile double result = a * b;
    return finite(result, operation, context);
}
inline double divide(double a, double b, char const* operation, ArithmeticContext context) {
    volatile double result = a / b;
    return finite(result, operation, context);
}
} // namespace detail
} // namespace PDE
#endif
