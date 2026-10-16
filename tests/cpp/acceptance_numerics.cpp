#include "acceptance_test_support.h"
#include <cfenv>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace {
using namespace acceptance;
using acceptance_reference::Data;
using acceptance_reference::Errors;
using acceptance_reference::Function;

void identity() {
    const char* source=std::getenv("C2C_SOURCE_REVISION");
#ifdef PDE_ACCEPTANCE_SOURCE_REVISION
    if (!source) source=PDE_ACCEPTANCE_SOURCE_REVISION;
#endif
    const std::string revision=source?source:"UNRECORDED";
    EXPECT_EQ(revision.size(),40u) << "Set C2C_SOURCE_REVISION to git rev-parse HEAD, or supply PDE_ACCEPTANCE_SOURCE_REVISION";
    EXPECT_TRUE(std::all_of(revision.begin(),revision.end(),[](unsigned char c) {
        return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');
    }));
    std::cout<<"C2C run=C2C-20261016-01 source="<<revision;
#if defined(__clang__)
    std::cout<<" compiler=Clang-"<<__clang_version__;
#elif defined(__GNUC__)
    std::cout<<" compiler=GCC-"<<__VERSION__;
#elif defined(_MSC_FULL_VER)
    std::cout<<" compiler=MSVC-"<<_MSC_FULL_VER;
#else
    std::cout<<" compiler=unknown";
#endif
#ifdef NDEBUG
    std::cout<<" build=Release(NDEBUG)";
#else
    std::cout<<" build=Debug(no-NDEBUG)";
#endif
    std::cout<<" cplusplus="<<__cplusplus<<" double_digits="<<std::numeric_limits<double>::digits<<std::endl;
}

void report(char const* fixture,PDE::Grid1D const& grid,double theta,Errors error) {
    std::cout<<std::setprecision(17)<<"C2C fixture="<<fixture<<" xL="<<grid.domain().xL
             <<" xR="<<grid.domain().xR<<" t0="<<grid.domain().t0<<" t1="<<grid.domain().t1
             <<" nx="<<grid.nx()<<" nt="<<grid.nt()<<" dx="<<grid.dx()<<" dt="<<grid.dt()
             <<" theta="<<theta<<" E_inf="<<error.infinity<<" E_2="<<error.l2<<std::endl;
}

void refinement(char const* name,Data const& fixture,Function const& exact,double t1,double theta,
                std::array<std::size_t,3> const& nx,std::array<std::size_t,3> const& nt,
                double minimumRate,double maximumRate,double maxInfinity,double maxL2) {
    std::array<Errors,3> errors{};
    for (std::size_t level=0;level<3;++level) {
        SCOPED_TRACE(::testing::Message()<<name<<" theta "<<theta<<" nx "<<nx[level]<<" nt "<<nt[level]);
        PDE::Grid1D grid(PDE::Domain1D(0,1,0,t1),nx[level],nt[level]);
        auto p=data(fixture); auto solver=PDE::makeSolver1D(grid,p); solver.setTheta(theta);
        const auto solution=solver.solve(); ASSERT_EQ(solution.size(),nx[level]);
        errors[level]=acceptance_reference::errors(values(solution),0,1,t1,exact);
        report(name,grid,theta,errors[level]);
        ASSERT_TRUE(std::isfinite(errors[level].infinity)); ASSERT_GT(errors[level].infinity,0);
        ASSERT_TRUE(std::isfinite(errors[level].l2)); ASSERT_GT(errors[level].l2,0);
        EXPECT_DOUBLE_EQ(solution[0],fixture.left(t1,0));
        EXPECT_DOUBLE_EQ(solution[nx[level]-1],fixture.right(t1,1));
    }
    for (std::size_t pair=0;pair<2;++pair) {
        const double pinfinity=std::log2(errors[pair].infinity/errors[pair+1].infinity);
        const double pl2=std::log2(errors[pair].l2/errors[pair+1].l2);
        std::cout<<std::setprecision(17)<<"C2C fixture="<<name<<" theta="<<theta<<" pair="<<pair
                 <<" p_inf="<<pinfinity<<" p_2="<<pl2<<std::endl;
        EXPECT_TRUE(std::isfinite(pinfinity)); EXPECT_GE(pinfinity,minimumRate); EXPECT_LE(pinfinity,maximumRate);
        EXPECT_TRUE(std::isfinite(pl2)); EXPECT_GE(pl2,minimumRate); EXPECT_LE(pl2,maximumRate);
    }
    EXPECT_LE(errors.back().infinity,maxInfinity); EXPECT_LE(errors.back().l2,maxL2);
}

TEST(AcceptanceNumerics, AC018_Binary64AndReproducibleBuildIdentity) {
    identity();
    EXPECT_EQ(std::numeric_limits<double>::digits,53); EXPECT_TRUE(std::numeric_limits<double>::is_iec559);
    EXPECT_EQ(std::fegetround(),FE_TONEAREST);
    volatile double tiny=std::numeric_limits<double>::denorm_min();
    EXPECT_GT(tiny*1.0,0.0);
}

TEST(AcceptanceNumerics, AC019_SpatialConvergenceAllTerms) {
    const double pi=std::acos(-1.0);
    const Function exact=[=](double t,double x) { return std::exp(t)*(1+x+std::sin(pi*x)); };
    Data fixture;
    fixture.a=[](double t,double x) { return 1+0.2*t+0.1*x; };
    fixture.b=[](double t,double x) { return 0.3+0.1*t-0.2*x; };
    fixture.c=[](double t,double x) { return -0.4+0.2*t+0.1*x; };
    fixture.f=[=](double t,double x) {
        const double a=1+0.2*t+0.1*x,b=0.3+0.1*t-0.2*x,c=-0.4+0.2*t+0.1*x;
        const double p=1+x+std::sin(pi*x);
        return std::exp(t)*((1-c)*p+a*pi*pi*std::sin(pi*x)-b*(1+pi*std::cos(pi*x)));
    };
    fixture.initial=exact; fixture.left=exact; fixture.right=exact;
    refinement("spatial_all_terms",fixture,exact,0.1,0.5,{{21,41,81}},{{4096,4096,4096}},1.85,2.15,2e-4,2e-4);
}

TEST(AcceptanceNumerics, AC020_CrankNicolsonTemporalConvergence) {
    const Function exact=[](double t,double x) { return std::exp(t)*(1+x+x*x); };
    Data fixture;
    fixture.a=[](double t,double x) { return 1+0.2*t+0.1*x; };
    fixture.b=[](double t,double x) { return 0.3+0.1*t-0.2*x; };
    fixture.c=[](double t,double x) { return -0.4+0.2*t+0.1*x; };
    fixture.f=[](double t,double x) {
        const double a=1+0.2*t+0.1*x,b=0.3+0.1*t-0.2*x,c=-0.4+0.2*t+0.1*x;
        return std::exp(t)*((1-c)*(1+x+x*x)-2*a-b*(1+2*x));
    };
    fixture.initial=exact; fixture.left=exact; fixture.right=exact;
    refinement("cn_temporal",fixture,exact,0.2,0.5,{{41,41,41}},{{20,40,80}},1.8,2.2,2e-6,2e-6);
}

TEST(AcceptanceNumerics, AC021_NondefaultThetaTemporalConvergence) {
    const Function exact=[](double t,double x) { return std::exp(t)*x*(1-x); };
    Data fixture;
    fixture.a=[](double,double) { return 1.0; };
    fixture.b=[](double,double) { return 0.0; }; fixture.c=fixture.b;
    fixture.f=[](double t,double x) { return std::exp(t)*(x*(1-x)+2); };
    fixture.initial=exact; fixture.left=exact; fixture.right=exact;
    for (double theta : {0.75,1.0})
        refinement("nondefault_theta_temporal",fixture,exact,0.2,theta,{{41,41,41}},{{10,20,40}},0.85,1.15,1e-4,7e-5);
}

TEST(AcceptanceNumerics, AC022_FixedHeatBenchmark) {
    const double pi=std::acos(-1.0);
    PDE::Grid1D grid(PDE::Domain1D(0,1,0,0.1),101,1000);
    auto p=constantData();
    p.diffusion=[](double,double) { return 1.0; };
    p.advection=[](double,double) { return 0.0; }; p.reaction=p.advection; p.source=p.advection;
    p.initial=[=](double,double x) { return std::sin(pi*x); };
    p.boundaryL=p.advection; p.boundaryR=p.advection;
    const auto solution=PDE::makeSolver1D(grid,p).solve(); ASSERT_EQ(solution.size(),101u);
    const auto error=acceptance_reference::errors(values(solution),0,1,0.1,[=](double t,double x) {
        return std::exp(-pi*pi*t)*std::sin(pi*x);
    });
    report("fixed_heat",grid,0.5,error);
    EXPECT_TRUE(std::isfinite(error.infinity)); EXPECT_TRUE(std::isfinite(error.l2));
    EXPECT_LE(error.infinity,4e-5); EXPECT_LE(error.l2,3e-5);
    EXPECT_DOUBLE_EQ(solution[0],0); EXPECT_DOUBLE_EQ(solution[100],0);
}

TEST(AcceptanceNumerics, AC023_HighHeatModeAmplificationAndStability) {
    const double pi=std::acos(-1.0),h=1.0/20;
    const double lambda=-4*std::pow(std::sin(19*pi/40),2)/(h*h);
    PDE::Grid1D grid(PDE::Domain1D(0,1,0,1),21,1);
    auto p=constantData();
    p.diffusion=[](double,double) { return 1.0; };
    p.advection=[](double,double) { return 0.0; }; p.reaction=p.advection; p.source=p.advection;
    p.boundaryL=p.advection; p.boundaryR=p.advection;
    p.initial=[=](double,double x) { return std::sin(19*pi*x); };
    auto solver=PDE::makeSolver1D(grid,p);
    for (double theta : {0.5,0.75,1.0}) {
        const double amplification=(1+(1-theta)*lambda)/(1-theta*lambda);
        EXPECT_LE(std::abs(amplification),1);
        solver.setTheta(theta); const auto solution=solver.solve(); ASSERT_EQ(solution.size(),21u);
        const Function exact=[=](double,double x) { return amplification*std::sin(19*pi*x); };
        const auto error=acceptance_reference::errors(values(solution),0,1,1,exact);
        report("high_heat_mode",grid,theta,error);
        for (std::size_t i=0;i<grid.nx();++i) EXPECT_NEAR(solution[i],exact(1,grid.x(i)),2e-11)<<i;
        EXPECT_DOUBLE_EQ(solution[0],0); EXPECT_DOUBLE_EQ(solution[20],0);
    }
}

TEST(AcceptanceNumerics, AC024_DenseIndependentMultistepOracle) {
    const auto fixture=acceptance_reference::mixedData();
    PDE::Grid1D grid(PDE::Domain1D(-0.5,1.3,-0.2,0.3),7,4);
    auto p=data(fixture); auto solver=PDE::makeSolver1D(grid,p);
    for (double theta : {0.5,std::nextafter(0.5,1.0),0.75,1.0}) {
        SCOPED_TRACE(theta); solver.setTheta(theta);
        const auto expected=acceptance_reference::evolve(fixture,-0.5,1.3,-0.2,0.3,7,4,theta);
        const auto solution=solver.solve(); near(solution,expected,2e-12);
        EXPECT_DOUBLE_EQ(solution[0],fixture.left(0.3,-0.5));
        EXPECT_DOUBLE_EQ(solution[6],fixture.right(0.3,1.3));
    }
}
} // namespace
