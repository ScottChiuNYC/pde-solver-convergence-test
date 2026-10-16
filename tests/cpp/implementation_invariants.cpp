#include <pde_software_replication_core/PDE.h>
#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

TEST(ImplementationInvariants, ReducedPivotIsCheckedBeforeRhsArithmetic) {
    PDE::Array1D lower(1, 2), diagonal(2, 1), upper(1, 1), rhs(2, 1);
    diagonal[1] = 2;
    rhs[0] = std::numeric_limits<double>::max();
    // The second pivot is zero; computing its reduced RHS first would overflow.
    try {
        PDE::TridiagonalSolver::solve(lower, diagonal, upper, rhs);
        FAIL() << "expected zero pivot rejection";
    } catch (std::runtime_error const& error) {
        const std::string message(error.what());
        EXPECT_NE(message.find("Thomas pivot"), std::string::npos);
        EXPECT_NE(message.find("row/index 1"), std::string::npos);
    }
    EXPECT_EQ(rhs[0], std::numeric_limits<double>::max());
    EXPECT_EQ(rhs[1], 1);
    EXPECT_EQ(lower[0], 2); EXPECT_EQ(diagonal[1], 2); EXPECT_EQ(upper[0], 1);
}

TEST(ImplementationInvariants, AffineSolutionWithVariableCoefficientsAndNonunitCoordinates) {
    PDE::Grid1D grid(PDE::Domain1D(-1.5, 2.5, -0.3, 0.4), 17, 7);
    PDE::PDE1D pde;
    auto exact = [](double t, double x) { return 4 + x + 2 * t; };
    pde.diffusion = [](double t, double x) { return 2 + 0.1 * t + 0.2 * x; };
    pde.advection = [](double t, double x) { return -3 + t - 0.5 * x; };
    pde.reaction = [](double, double x) { return 0.2 + 0.1 * x; };
    pde.source = [exact](double t, double x) {
        return 2 - (-3 + t - 0.5 * x) - (0.2 + 0.1 * x) * exact(t, x);
    };
    pde.initial = exact; pde.boundaryL = exact; pde.boundaryR = exact;
    const auto result = PDE::makeSolver1D(grid, pde).solve();
    ASSERT_EQ(result.size(), grid.nx());
    for (std::size_t i = 0; i < result.size(); ++i)
        EXPECT_NEAR(result[i], exact(grid.domain().t1, grid.x(i)), 3e-13) << "node " << i;
    EXPECT_EQ(result[0], exact(0.4, -1.5)); EXPECT_EQ(result[result.size()-1], exact(0.4, 2.5));
}

TEST(ImplementationInvariants, ValidationDiagnosticsIdentifyRoleAndCoordinates) {
    PDE::Grid1D grid(PDE::Domain1D(0, 2, 1, 2), 3, 1);
    PDE::PDE1D pde;
    pde.initial = pde.boundaryL = pde.boundaryR = [](double, double) { return 0; };
    pde.diffusion = [](double, double) { return -1; };
    try {
        (void)PDE::makeSolver1D(grid, pde).solve();
        FAIL() << "expected sampled diffusion rejection";
    } catch (std::runtime_error const& error) {
        const std::string message(error.what());
        EXPECT_NE(message.find("diffusion"), std::string::npos);
        EXPECT_NE(message.find("t=1."), std::string::npos);
        EXPECT_NE(message.find("x=1."), std::string::npos);
    }
}
