#include "acceptance_test_support.h"

namespace {
using namespace acceptance;

TEST(AcceptanceArithmetic, AC025_OverflowSafeMeansAndGradualUnderflow) {
    const double maximum=std::numeric_limits<double>::max();
    PDE::Grid1D grid(PDE::Domain1D(0,std::ldexp(1.0,500),0,1),3,1);
    for (int role=0;role<3;++role) for (double sign : {1.0,-1.0}) {
        if (role==0&&sign<0) continue;
        auto p=constantData(0);
        if (role==0) p.diffusion=[=](double,double) { return maximum; };
        if (role==1) p.advection=[=](double,double) { return sign*maximum; };
        if (role==2) p.reaction=[=](double,double) { return sign*maximum; };
        auto solver=PDE::makeSolver1D(grid,p);
        PDE::Array1D result; ASSERT_NO_THROW(result=solver.solve()); near(result,{0,0,0},0);
    }
    // Squaring dx would overflow, but the prescribed two divisions are finite.
    PDE::Grid1D wide(PDE::Domain1D(0,std::ldexp(1.0,601),0,1),3,1);
    auto p=constantData(); auto solver=PDE::makeSolver1D(wide,p);
    PDE::Array1D result; ASSERT_NO_THROW(result=solver.solve()); near(result,{1,1,1},0);
}

TEST(AcceptanceArithmetic, AC026_GeneratedPDEOverflowsAndZeroImplicitPivot) {
    const double maximum=std::numeric_limits<double>::max();
    {
        PDE::Grid1D grid(PDE::Domain1D(0,1,0,1),3,1); auto p=constantData(0);
        p.diffusion=[=](double,double) { return maximum; };
        auto solver=PDE::makeSolver1D(grid,p); runtimeFailure([&] { (void)solver.solve(); });
        // The same referenced scheme can be used after a failure; no persistent corrupted cache.
        p.diffusion=[](double,double) { return 1.0; };
        near(solver.solve(),{0,0,0},0);
    }
    {
        PDE::Grid1D grid(PDE::Domain1D(0,2,0,1),3,1); auto p=constantData(0);
        p.initial=[](double,double) { return 1.0; }; p.reaction=[](double,double) { return 4.0; };
        auto solver=PDE::makeSolver1D(grid,p); runtimeFailure([&] { (void)solver.solve(); });
    }
    {
        // qD must form -2*ar before adding cdt: a fused cancellation is forbidden.
        PDE::Grid1D grid(PDE::Domain1D(0,2,0,1),3,1); auto p=constantData(0);
        p.diffusion=[=](double,double) { return maximum; };
        p.reaction=[=](double,double) { return maximum; };
        auto solver=PDE::makeSolver1D(grid,p); runtimeFailure([&] { (void)solver.solve(); });
    }
    {
        // Weighted old-state products are checked before canceling RHS sums.
        PDE::Grid1D grid(PDE::Domain1D(0,2,0,1),3,1); auto p=constantData(0);
        p.diffusion=[](double,double) { return 8.0; };
        p.boundaryL=[=](double t,double) { return t==0?maximum:0.0; };
        p.boundaryR=[=](double t,double) { return t==0?-maximum:0.0; };
        auto solver=PDE::makeSolver1D(grid,p); runtimeFailure([&] { (void)solver.solve(); });
    }
    {
        // The source term's outer dt product overflows after finite weighted samples.
        PDE::Grid1D grid(PDE::Domain1D(0,2,0,4),3,1); auto p=constantData(0);
        p.source=[=](double,double) { return maximum; };
        auto solver=PDE::makeSolver1D(grid,p); runtimeFailure([&] { (void)solver.solve(); });
    }
    {
        // Endpoint boundary correction must be checked as its own product.
        PDE::Grid1D grid(PDE::Domain1D(0,2,0,1),3,1); auto p=constantData(0);
        p.diffusion=[](double,double) { return 8.0; };
        p.boundaryL=[=](double t,double) { return t==1?maximum:0.0; };
        auto solver=PDE::makeSolver1D(grid,p); runtimeFailure([&] { (void)solver.solve(); });
    }
    {
        // Left correction comes first: max - (-max/2) overflows before the
        // opposite right correction could cancel it.
        PDE::Grid1D grid(PDE::Domain1D(0,2,0,1),3,1); auto p=constantData(0);
        p.source=[=](double,double) { return maximum; };
        p.boundaryL=[=](double t,double) { return t==1?maximum:0.0; };
        p.boundaryR=[=](double t,double) { return t==1?-maximum:0.0; };
        auto solver=PDE::makeSolver1D(grid,p); runtimeFailure([&] { (void)solver.solve(); });
    }
}

TEST(AcceptanceArithmetic, AC027_PrescribedLeftToRightRHSAdditionCannotBeReassociated) {
    const double maximum=std::numeric_limits<double>::max();
    PDE::Grid1D grid(PDE::Domain1D(0,2,0,1),3,1); auto p=constantData(0);
    // a=1, c=3 gives qL=qR=1 and qD=1. Old products, in order,
    // are max/2, 3*max/4, -max/2. The FIRST sum overflows although
    // reassociation could yield a finite final RHS.
    p.reaction=[](double,double) { return 3.0; };
    p.initial=[=](double,double) { return maximum/2; };
    p.boundaryL=[=](double t,double) { return t==0?maximum:0.0; };
    p.boundaryR=[=](double t,double) { return t==0?-maximum:0.0; };
    auto solver=PDE::makeSolver1D(grid,p); runtimeFailure([&] { (void)solver.solve(); });
}
} // namespace
