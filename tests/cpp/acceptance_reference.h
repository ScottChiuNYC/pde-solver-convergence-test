#ifndef C2C_ACCEPTANCE_REFERENCE_H
#define C2C_ACCEPTANCE_REFERENCE_H

// Independent pre-implementation mathematical assets. No production headers.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace acceptance_reference {
using Function = std::function<double(double, double)>;

struct Data {
    Function a, b, c, f, initial, left, right;
};

struct Errors { double infinity, l2; };

inline Errors errors(std::vector<double> const& values, double xL, double xR,
                     double t, Function const& exact) {
    if (values.size() < 2) throw std::runtime_error("oracle needs endpoints");
    const double h = (xR - xL) / static_cast<double>(values.size() - 1);
    double maximum = 0.0, sum = 0.0;
    for (std::size_t i = 0; i < values.size(); ++i) {
        const double x = i + 1 == values.size() ? xR : xL + static_cast<double>(i) * h;
        const double error = values[i] - exact(t, x);
        if (!std::isfinite(error)) throw std::runtime_error("nonfinite oracle error");
        maximum = std::max(maximum, std::abs(error));
        sum += error * error;
    }
    return {maximum, std::sqrt(h * sum)};
}

// Dense Gaussian elimination WITH partial pivoting in long double. This is an
// independent benign-fixture oracle, deliberately not Thomas elimination.
inline std::vector<double> denseSolve(std::vector<std::vector<long double>> matrix,
                                      std::vector<long double> rhs) {
    const std::size_t n = rhs.size();
    if (matrix.size() != n || n == 0) throw std::runtime_error("oracle shape");
    for (auto const& row : matrix)
        if (row.size() != n) throw std::runtime_error("oracle row shape");
    for (std::size_t k = 0; k < n; ++k) {
        std::size_t pivot = k;
        for (std::size_t j = k + 1; j < n; ++j)
            if (std::abs(matrix[j][k]) > std::abs(matrix[pivot][k])) pivot = j;
        if (matrix[pivot][k] == 0) throw std::runtime_error("singular oracle matrix");
        std::swap(matrix[k], matrix[pivot]);
        std::swap(rhs[k], rhs[pivot]);
        for (std::size_t j = k + 1; j < n; ++j) {
            const long double multiplier = matrix[j][k] / matrix[k][k];
            for (std::size_t q = k; q < n; ++q)
                matrix[j][q] -= multiplier * matrix[k][q];
            rhs[j] -= multiplier * rhs[k];
        }
    }
    std::vector<long double> solution(n);
    for (std::size_t reverse = n; reverse-- > 0;) {
        long double value = rhs[reverse];
        for (std::size_t j = reverse + 1; j < n; ++j)
            value -= matrix[reverse][j] * solution[j];
        solution[reverse] = value / matrix[reverse][reverse];
    }
    return std::vector<double>(solution.begin(), solution.end());
}

inline double endpointMean(double p, double q) {
    const double half = std::numeric_limits<double>::max() / 2;
    return std::abs(p) <= half && std::abs(q) <= half ? (p + q) / 2 : p / 2 + q / 2;
}

// Separate dense spatial-operator assembly; only used for finite, well-scaled
// small grids. Required negative arithmetic cases have direct assertions.
inline std::vector<double> evolve(Data const& data, double xL, double xR,
                                  double t0, double t1, std::size_t nx,
                                  std::size_t nt, double theta) {
    const double h = (xR - xL) / static_cast<double>(nx - 1);
    const double dt = (t1 - t0) / static_cast<double>(nt);
    const double r = (dt / h) / h, s = (dt / h) / 2;
    const auto coordinate = [=](std::size_t i) {
        return i + 1 == nx ? xR : xL + static_cast<double>(i) * h;
    };
    const auto time = [=](std::size_t k) {
        return k == nt ? t1 : t0 + static_cast<double>(k) * dt;
    };
    std::vector<double> old(nx);
    old.front() = data.left(t0, xL);
    old.back() = data.right(t0, xR);
    for (std::size_t i = 1; i + 1 < nx; ++i) old[i] = data.initial(t0, coordinate(i));
    const std::size_t m = nx - 2;
    for (std::size_t k = 0; k < nt; ++k) {
        std::vector<std::vector<long double>> matrix(m, std::vector<long double>(m));
        std::vector<long double> rhs(m);
        const double left = data.left(time(k + 1), xL), right = data.right(time(k + 1), xR);
        for (std::size_t j = 0; j < m; ++j) {
            const std::size_t i = j + 1;
            const double x = coordinate(i);
            const double ar = endpointMean(data.a(time(k), x), data.a(time(k + 1), x)) * r;
            const double bs = endpointMean(data.b(time(k), x), data.b(time(k + 1), x)) * s;
            const double cdt = endpointMean(data.c(time(k), x), data.c(time(k + 1), x)) * dt;
            const double qL = ar - bs, qD = (-2 * ar) + cdt, qR = ar + bs;
            const double aL = -theta * qL, aD = 1 - theta * qD, aR = -theta * qR;
            const double termL = (1 - theta) * qL * old[i - 1];
            const double termD = (1 + (1 - theta) * qD) * old[i];
            const double termR = (1 - theta) * qR * old[i + 1];
            const double oldSource = theta == 1 ? 0 : (1 - theta) * data.f(time(k), x);
            const double source = dt * (oldSource + theta * data.f(time(k + 1), x));
            double value = ((termL + termD) + termR) + source;
            if (j == 0) value -= aL * left;
            if (j + 1 == m) value -= aR * right;
            matrix[j][j] = aD;
            if (j > 0) matrix[j][j - 1] = aL;
            if (j + 1 < m) matrix[j][j + 1] = aR;
            rhs[j] = value;
        }
        auto interior = denseSolve(std::move(matrix), std::move(rhs));
        old.front() = left;
        old.back() = right;
        std::copy(interior.begin(), interior.end(), old.begin() + 1);
    }
    return old;
}

inline Data mixedData() {
    return {
        [](double t, double x) { return 1 + 0.2 * t + 0.1 * x; },
        [](double t, double x) { return 0.3 + 0.1 * t - 0.2 * x; },
        [](double t, double x) { return -0.4 + 0.2 * t + 0.1 * x; },
        [](double t, double x) { return 0.4 + t * t + 0.3 * x; },
        [](double, double x) { return 1 + x + x * x; },
        [](double t, double x) { return 1 + 2 * t + 0.25 * x; },
        [](double t, double x) { return 2 - t + 0.5 * x; }
    };
}
} // namespace acceptance_reference
#endif
