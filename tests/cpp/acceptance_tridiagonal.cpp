#include "acceptance_test_support.h"
#include <limits>

namespace {
using namespace acceptance;

void failsUnchanged(PDE::Array1D& lower, PDE::Array1D& diagonal,
                    PDE::Array1D& upper, PDE::Array1D& rhs) {
    const auto l = bits(lower), d = bits(diagonal), u = bits(upper), b = bits(rhs);
    runtimeFailure([&] { PDE::TridiagonalSolver::solve(lower,diagonal,upper,rhs); });
    EXPECT_EQ(bits(lower),l); EXPECT_EQ(bits(diagonal),d); EXPECT_EQ(bits(upper),u); EXPECT_EQ(bits(rhs),b);
}

void success(std::vector<double> l, std::vector<double> d, std::vector<double> u,
             std::vector<double> b, std::vector<double> const& expected, double scale) {
    for (auto* values : {&l,&d,&u,&b}) for (double& value : *values) value *= scale;
    auto lower=array(l), diagonal=array(d), upper=array(u), rhs=array(b);
    const auto lb=bits(lower), db=bits(diagonal), ub=bits(upper);
    ASSERT_NO_THROW(PDE::TridiagonalSolver::solve(lower,diagonal,upper,rhs));
    near(rhs,expected,1e-12);
    EXPECT_EQ(bits(lower),lb); EXPECT_EQ(bits(diagonal),db); EXPECT_EQ(bits(upper),ub);
}

TEST(AcceptanceThomas, AC006_SuccessAndUniformScaling) {
    for (int exponent : {0,-500,500}) {
        SCOPED_TRACE(exponent);
        const double scale = std::ldexp(1.0,exponent);
        success({}, {2}, {}, {6}, {3}, scale);
        success({1,1},{4,4,4},{1,1},{6,12,14},{1,2,3},scale);
    }
    // Sharing the two off-diagonal input objects is explicitly supported.
    auto shared=array({1,1}), diagonal=array({4,4,4}), rhs=array({6,12,14});
    const auto before=bits(shared);
    ASSERT_NO_THROW(PDE::TridiagonalSolver::solve(shared,diagonal,shared,rhs));
    near(rhs,{1,2,3},1e-12); EXPECT_EQ(bits(shared),before);
    // RHS does not enter the row scale, and there is no absolute pivot floor.
    success({}, {std::numeric_limits<double>::denorm_min()}, {}, {std::numeric_limits<double>::denorm_min()}, {1},1);
    success({}, {1}, {}, {std::numeric_limits<double>::max()}, {std::numeric_limits<double>::max()},1);
}

TEST(AcceptanceThomas, AC007_ShapesAndAliasingAreTransactional) {
    {
        PDE::Array1D l,d,u,b; failsUnchanged(l,d,u,b);
    }
    for (int which=0; which<6; ++which) {
        SCOPED_TRACE(which);
        auto l=array({1,1}), d=array({4,4,4}), u=array({1,1}), b=array({6,12,14});
        if (which==0) l=PDE::Array1D(1,1);
        if (which==1) l=PDE::Array1D(3,1);
        if (which==2) u=PDE::Array1D(1,1);
        if (which==3) u=PDE::Array1D(3,1);
        if (which==4) b=PDE::Array1D(2,1);
        if (which==5) b=PDE::Array1D(4,1);
        failsUnchanged(l,d,u,b);
    }
    // Same-object aliasing must be rejected, including diagonal/RHS with valid shapes.
    {
        auto empty=array({}), diagonal=array({2}); const auto before=bits(diagonal);
        runtimeFailure([&] { PDE::TridiagonalSolver::solve(empty,diagonal,empty,diagonal); });
        EXPECT_EQ(bits(diagonal),before);
    }
    for (bool aliasLower : {true,false}) {
        auto shared=array({1}), diagonal=array({2,2}), other=array({1});
        const auto s=bits(shared), d=bits(diagonal), o=bits(other);
        runtimeFailure([&] {
            if (aliasLower) PDE::TridiagonalSolver::solve(shared,diagonal,other,shared);
            else PDE::TridiagonalSolver::solve(other,diagonal,shared,shared);
        });
        EXPECT_EQ(bits(shared),s); EXPECT_EQ(bits(diagonal),d); EXPECT_EQ(bits(other),o);
    }
}

TEST(AcceptanceThomas, AC008_NonfiniteInputsAndUnusablePivots) {
    for (int role=0; role<4; ++role) for (double invalid : {
         std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),
         -std::numeric_limits<double>::infinity()}) {
        SCOPED_TRACE(role);
        auto l=array({1,1}), d=array({4,4,4}), u=array({1,1}), b=array({6,12,14});
        std::array<PDE::Array1D*,4> inputs{{&l,&d,&u,&b}};
        (*inputs[role])[inputs[role]->size()-1]=invalid;
        failsUnchanged(l,d,u,b);
    }
    for (int fixture=0; fixture<4; ++fixture) {
        SCOPED_TRACE(fixture);
        auto l=array({1}), d=array({1,1}), u=array({1}), b=array({2,2});
        if (fixture==0) { l[0]=0; d[0]=0; u[0]=0; } // zero original row
        if (fixture==1) d[0]=0; // nonsingular but requires pivoting: forbidden
        if (fixture==2) { l[0]=1; d[0]=1; d[1]=1; u[0]=1; } // reduced zero pivot
        if (fixture==3) { l[0]=0; d[0]=1; d[1]=0; u[0]=1; } // later zero row
        failsUnchanged(l,d,u,b);
    }
}

TEST(AcceptanceThomas, AC009_OriginalRowThresholdAndExactEquality) {
    const double tau=64*std::numeric_limits<double>::epsilon();
    for (int exponent : {0,-500,500}) {
        const double scale=std::ldexp(1.0,exponent);
        for (int power : {-48,-46,-44}) {
            SCOPED_TRACE(::testing::Message()<<"scale exponent "<<exponent<<" delta exponent "<<power);
            const double delta=std::ldexp(1.0,power);
            if (power==-44) success({1},{1,1+delta},{1},{2,2+delta},{1,1},scale);
            else {
                auto l=array({scale}), d=array({scale,(1+delta)*scale}), u=array({scale}),
                     b=array({2*scale,(2+delta)*scale});
                failsUnchanged(l,d,u,b);
            }
        }
        auto l=array({scale}), d=array({scale,scale}), u=array({(1-tau)*scale}),
             b=array({(2-tau)*scale,2*scale});
        failsUnchanged(l,d,u,b);
    }
    // rho uses the ORIGINAL lower entry even after elimination changes the pivot.
    auto l=array({1}), d=array({1,1}), u=array({1-tau/2}), b=array({2,2});
    failsUnchanged(l,d,u,b);
}

TEST(AcceptanceThomas, AC010_EveryGeneratedOperationIsCheckedTransactionally) {
    const double maximum=std::numeric_limits<double>::max(), tiny=std::numeric_limits<double>::denorm_min();
    { auto l=array({}),d=array({tiny}),u=array({}),b=array({maximum}); failsUnchanged(l,d,u,b); }
    // Nonfinite multiplier; first pivot passes its original row's relative policy.
    { auto l=array({maximum}),d=array({tiny,maximum}),u=array({0}),b=array({0,0}); failsUnchanged(l,d,u,b); }
    // Nonfinite w*upper must be rejected before subtracting it.
    { auto l=array({std::ldexp(1.0,1022)}),d=array({std::ldexp(1.0,980),maximum}),
           u=array({std::ldexp(1.0,1022)}),b=array({0,0}); failsUnchanged(l,d,u,b); }
    // Fused cancellation would make max - 2*max finite; separate product fails.
    { auto l=array({maximum}),d=array({maximum/2,maximum}),u=array({maximum}),b=array({0,0}); failsUnchanged(l,d,u,b); }
    // Nonfinite w*y, despite a finite new pivot.
    { auto l=array({2}),d=array({1,3}),u=array({0}),b=array({maximum,maximum}); failsUnchanged(l,d,u,b); }
    // Back-substitution product overflows after a finite final-row division.
    { auto l=array({0}),d=array({1,1}),u=array({2}),b=array({maximum,maximum}); failsUnchanged(l,d,u,b); }
    // Back-substitution subtraction overflows although each product is finite.
    { auto l=array({0}),d=array({1,1}),u=array({-1}),b=array({maximum,maximum}); failsUnchanged(l,d,u,b); }
    // Late reduced-pivot subtraction overflow preserves all inputs too.
    { auto l=array({maximum}),d=array({maximum/2,maximum}),u=array({-maximum/2}),b=array({0,0}); failsUnchanged(l,d,u,b); }
}
} // namespace
