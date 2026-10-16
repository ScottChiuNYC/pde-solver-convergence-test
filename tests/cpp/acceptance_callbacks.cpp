#include "acceptance_test_support.h"
#include <future>
#include <set>
#include <utility>

namespace {
using namespace acceptance;

bool isInterior(PDE::Grid1D const& grid,double x) {
    for (std::size_t i=1;i+1<grid.nx();++i) if (x==grid.x(i)) return true;
    return false;
}
bool isTime(PDE::Grid1D const& grid,double t) {
    for (std::size_t k=0;k<=grid.nt();++k) if (t==grid.t(k)) return true;
    return false;
}

TEST(AcceptanceCallbacks, AC011_MissingCallbacksFailBeforeAnyInvocation) {
    PDE::Grid1D grid(PDE::Domain1D(0,1,0,0.2),5,2);
    { PDE::PDE1D p; auto solver=PDE::makeSolver1D(grid,p); runtimeFailure([&] { (void)solver.solve(); }); }
    for (std::size_t missing=0;missing<roles().size();++missing) {
        SCOPED_TRACE(roleNames()[missing]);
        auto p=constantData(); int calls=0;
        for (auto role : roles()) p.*role=[&](double,double) { ++calls; return 1.0; };
        p.*roles()[missing]={};
        auto solver=PDE::makeSolver1D(grid,p);
        runtimeFailure([&] { (void)solver.solve(); }); EXPECT_EQ(calls,0);
        // Mutable fields are validated on every solve, never silently restored.
        EXPECT_FALSE(p.*roles()[missing]);
    }
}

TEST(AcceptanceCallbacks, AC012_UnsupportedBoundariesFailBeforeAnyInvocation) {
    PDE::Grid1D grid(PDE::Domain1D(0,1,0,0.2),5,2);
    for (std::size_t side=0;side<2;++side) for (auto type : {
         PDE::BoundaryType::Neumann,PDE::BoundaryType::Gamma,
         static_cast<PDE::BoundaryType>(-1),static_cast<PDE::BoundaryType>(99)}) {
        auto p=constantData(); int calls=0;
        for (auto role : roles()) p.*role=[&](double,double) { ++calls; return 1.0; };
        auto solver=PDE::makeSolver1D(grid,p);
        p.boundaryConfig.types[side]=type;
        runtimeFailure([&] { (void)solver.solve(); }); EXPECT_EQ(calls,0);
    }
}

TEST(AcceptanceCallbacks, AC013_AllRequiredSamplesAndOnlyPermittedSamples) {
    PDE::Grid1D grid(PDE::Domain1D(0,1,0,0.2),5,2);
    for (double theta : {0.5,0.75,1.0}) {
        SCOPED_TRACE(theta);
        auto p=constantData(); std::array<std::set<std::pair<double,double>>,7> seen;
        const auto members=roles();
        for (std::size_t role=0;role<members.size();++role) {
            p.*members[role]=[&,role](double t,double x) {
                bool allowed=false;
                if (role<=3) allowed=isTime(grid,t)&&isInterior(grid,x)&&(role!=3||theta<1||t!=grid.t(0));
                if (role==4) allowed=t==grid.t(0)&&isInterior(grid,x);
                if (role==5) allowed=isTime(grid,t)&&x==grid.domain().xL;
                if (role==6) allowed=isTime(grid,t)&&x==grid.domain().xR;
                if (!allowed) throw UserFailure{"outside required sample set: "+std::string(roleNames()[role])};
                seen[role].insert({t,x});
                return role==0||role>=4 ? 1.0 : 0.0;
            };
        }
        auto solver=PDE::makeSolver1D(grid,p); solver.setTheta(theta);
        PDE::Array1D solution;
        ASSERT_NO_THROW(solution=solver.solve()); near(solution,{1,1,1,1,1},1e-12);
        for (std::size_t role=0;role<members.size();++role) {
            std::set<std::pair<double,double>> expected;
            if (role<=3) for (std::size_t k=role==3&&theta==1 ? 1 : 0;k<=grid.nt();++k)
                for (std::size_t i=1;i+1<grid.nx();++i) expected.insert({grid.t(k),grid.x(i)});
            if (role==4) for (std::size_t i=1;i+1<grid.nx();++i) expected.insert({grid.t(0),grid.x(i)});
            if (role>=5) for (std::size_t k=0;k<=grid.nt();++k)
                expected.insert({grid.t(k),role==5?grid.domain().xL:grid.domain().xR});
            EXPECT_EQ(seen[role],expected)<<roleNames()[role];
        }
    }
}

TEST(AcceptanceCallbacks, AC014_AllRolesRejectEveryNonfiniteClassAtRequiredSample) {
    PDE::Grid1D grid(PDE::Domain1D(0,1,0,0.2),5,2);
    for (std::size_t role=0;role<roles().size();++role) for (double bad : {
         std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),
         -std::numeric_limits<double>::infinity()}) {
        SCOPED_TRACE(roleNames()[role]);
        auto p=constantData(); const auto original=p.*roles()[role];
        p.*roles()[role]=[&,role,bad,original](double t,double x) {
            bool selected=false;
            if (role<=3) selected=t==grid.t(1)&&x==grid.x(2);
            if (role==4) selected=t==grid.t(0)&&x==grid.x(2);
            if (role==5) selected=t==grid.t(grid.nt())&&x==grid.x(0);
            if (role==6) selected=t==grid.t(grid.nt())&&x==grid.x(grid.nx()-1);
            return selected ? bad : original(t,x);
        };
        auto solver=PDE::makeSolver1D(grid,p);
        runtimeFailure([&] { (void)solver.solve(); });
    }
    for (double diffusion : {0.0,-1.0}) {
        auto p=constantData(); p.diffusion=[&](double t,double x) {
            return t==grid.t(1)&&x==grid.x(2) ? diffusion : 1.0;
        };
        auto solver=PDE::makeSolver1D(grid,p); runtimeFailure([&] { (void)solver.solve(); });
    }
}

TEST(AcceptanceCallbacks, AC015_UserExceptionsPropagateWithOriginalTypePayloadAndNoRetry) {
    PDE::Grid1D grid(PDE::Domain1D(0,1,0,0.2),5,2);
    for (std::size_t role=0;role<roles().size();++role) {
        SCOPED_TRACE(roleNames()[role]); auto p=constantData(); int calls=0;
        const std::string payload="distinctive "+std::string(roleNames()[role]);
        p.*roles()[role]=[&](double,double)->double { ++calls; throw UserFailure{payload}; };
        auto solver=PDE::makeSolver1D(grid,p); bool caught=false;
        try { (void)solver.solve(); }
        catch (UserFailure const& failure) { caught=true; EXPECT_EQ(failure.payload,payload); }
        catch (...) { ADD_FAILURE()<<"callback exception was wrapped/substituted"; }
        EXPECT_TRUE(caught); EXPECT_EQ(calls,1);
        calls=0; caught=false;
        p.*roles()[role]=[&](double,double)->double { ++calls; throw UserStdFailure(payload); };
        try { (void)solver.solve(); }
        catch (UserStdFailure const& failure) {
            caught=true; EXPECT_EQ(std::string(failure.what()),payload); EXPECT_EQ(failure.marker,314159);
        }
        catch (...) { ADD_FAILURE()<<"standard callback exception was wrapped/substituted"; }
        EXPECT_TRUE(caught); EXPECT_EQ(calls,1);
        p.*roles()[role]=[](double,double)->double { throw std::bad_alloc(); };
        EXPECT_THROW((void)solver.solve(),std::bad_alloc);
    }
}

TEST(AcceptanceCallbacks, AC016_UnusedInitialEndpointsAndThetaOneOldSource) {
    PDE::Grid1D grid(PDE::Domain1D(0,2,0,1),3,1);
    auto p=constantData();
    p.initial=[](double t,double x) {
        if (t!=0||x!=1) throw UserFailure{"unused initial endpoint"};
        return 2.0;
    };
    p.diffusion=[](double t,double x) { if (x!=1||t<0||t>1) throw UserFailure{"unused diffusion"}; return 1+t; };
    p.advection=[](double t,double x) { if (x!=1) throw UserFailure{"unused advection"}; return 2*t; };
    p.reaction=[](double t,double x) { if (x!=1) throw UserFailure{"unused reaction"}; return -2*t; };
    p.boundaryL=[](double t,double x) { if (x!=0) throw UserFailure{"wrong left coordinate"}; return 1+2*t; };
    p.boundaryR=[](double t,double x) { if (x!=2) throw UserFailure{"wrong right coordinate"}; return 5-t; };
    auto solver=PDE::makeSolver1D(grid,p);
    for (double theta : {0.5,0.75,1.0}) {
        p.source=[=](double t,double x) {
            if (x!=1||(theta==1&&t==0)) throw UserFailure{"zero-weight old source"};
            return 2+4*t;
        };
        solver.setTheta(theta); PDE::Array1D out;
        ASSERT_NO_THROW(out=solver.solve()); ASSERT_EQ(out.size(),3u);
        EXPECT_DOUBLE_EQ(out[0],3); EXPECT_DOUBLE_EQ(out[2],4);
        EXPECT_NEAR(out[1],(7+12*theta)/(1+4*theta),1e-12);
    }
}

TEST(AcceptanceCallbacks, AC017_DefaultDataReferencesIndependentResultsAndRepeatability) {
    PDE::Grid1D grid(PDE::Domain1D(0,1,0,0.1),5,2); auto p=constantData();
    auto solver=PDE::makeSolver1D(grid,p); EXPECT_DOUBLE_EQ(solver.theta(),0.5);
    const auto first=solver.solve(); const auto firstBits=bits(first);
    near(first,{1,1,1,1,1},1e-12);
    const auto repeat=solver.solve(); EXPECT_EQ(bits(repeat),firstBits);
    p.initial=[](double,double) { return 2.0; };
    p.boundaryL=p.initial; p.boundaryR=p.initial;
    auto copied=solver; auto moved=std::move(copied);
    const auto second=moved.solve(); near(second,{2,2,2,2,2},1e-12); EXPECT_EQ(bits(first),firstBits);
    EXPECT_NE(first.data(),second.data()); EXPECT_DOUBLE_EQ(solver.theta(),0.5);
    p.source={}; runtimeFailure([&] { (void)solver.solve(); }); EXPECT_EQ(bits(first),firstBits);
    p.source=[](double,double) { return 0.0; };
    // Scheme retains the Grid object itself, not a captured grid value.
    grid=PDE::Grid1D(PDE::Domain1D(0,1,0,0.2),7,3);
    const auto rebound=solver.solve(); near(rebound,{2,2,2,2,2,2,2},1e-12);
    EXPECT_EQ(bits(first),firstBits); EXPECT_EQ(grid.nx(),7u); EXPECT_DOUBLE_EQ(grid.domain().t1,0.2);
}

TEST(AcceptanceCallbacks, AC029_ConcurrentReadOnlySolvesHaveIndependentStorage) {
    const auto fixture=acceptance_reference::mixedData();
    PDE::Grid1D grid(PDE::Domain1D(-0.5,1.3,-0.2,0.3),7,4); auto p=data(fixture);
    const auto solver=PDE::makeSolver1D(grid,p); const auto baseline=solver.solve();
    auto first=std::async(std::launch::async,[&] { return solver.solve(); });
    auto second=std::async(std::launch::async,[&] { return solver.solve(); });
    const auto a=first.get(),b=second.get();
    EXPECT_EQ(bits(a),bits(baseline)); EXPECT_EQ(bits(b),bits(baseline));
    EXPECT_NE(a.data(),b.data()); EXPECT_NE(a.data(),baseline.data()); EXPECT_DOUBLE_EQ(solver.theta(),0.5);
}
} // namespace
