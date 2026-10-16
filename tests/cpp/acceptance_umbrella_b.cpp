#include <pde_software_replication_core/PDE.h>
#include <pde_software_replication_core/PDE.h>
#include <gtest/gtest.h>
#include <cmath>
#include <vector>

std::vector<double> acceptanceUmbrellaOtherTranslationUnit();

TEST(AcceptanceIntegration, AC028_UmbrellaCxx17TwoTranslationUnits) {
    const auto solution=acceptanceUmbrellaOtherTranslationUnit();
    ASSERT_EQ(solution.size(),101u);
    const double pi=std::acos(-1.0);
    for (std::size_t i=0;i<solution.size();++i) {
        ASSERT_TRUE(std::isfinite(solution[i]));
        EXPECT_NEAR(solution[i],std::exp(-pi*pi*0.1)*std::sin(pi*double(i)/100),4e-5)<<i;
    }
    EXPECT_DOUBLE_EQ(solution.front(),0); EXPECT_DOUBLE_EQ(solution.back(),0);
    PDE::Array1D lower,diagonal(1,2),upper,rhs(1,6);
    PDE::TridiagonalSolver::solve(lower,diagonal,upper,rhs);
    EXPECT_NEAR(rhs[0],3,1e-12);
}
