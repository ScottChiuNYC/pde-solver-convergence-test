#include "acceptance_test_support.h"
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>

namespace {
using namespace acceptance;
using Scheme = PDE::Scheme::CrankNicholson1D;
static_assert(std::is_same<PDE::Func::F1D, std::function<double(double, double)>>::value, "callback API");
static_assert(std::is_copy_constructible<PDE::Domain1D>::value && std::is_copy_assignable<PDE::Domain1D>::value, "domain values");
static_assert(std::is_move_constructible<PDE::Grid1D>::value && std::is_move_assignable<PDE::Grid1D>::value, "grid values");
static_assert(std::is_copy_constructible<PDE::PDE1D>::value && std::is_copy_assignable<PDE::PDE1D>::value, "PDE values");
static_assert(std::is_copy_constructible<PDE::BoundaryConfig1D>::value && std::is_copy_assignable<PDE::BoundaryConfig1D>::value, "boundary values");
static_assert(std::is_copy_constructible<Scheme>::value && std::is_move_constructible<Scheme>::value, "scheme construction");
static_assert(!std::is_copy_assignable<Scheme>::value && !std::is_move_assignable<Scheme>::value, "scheme assignment is deleted");
static_assert(std::is_same<decltype(std::declval<PDE::Grid1D const&>().domain()), PDE::Domain1D const&>::value, "domain reference API");
static_assert(std::is_same<decltype(PDE::Domain1D::xL),double>::value && std::is_same<decltype(PDE::Domain1D::xR),double>::value &&
              std::is_same<decltype(PDE::Domain1D::t0),double>::value && std::is_same<decltype(PDE::Domain1D::t1),double>::value,"public double domain fields");
static_assert(std::is_same<decltype(std::declval<PDE::Grid1D const&>().nx()),std::size_t>::value &&
              std::is_same<decltype(std::declval<PDE::Grid1D const&>().nt()),std::size_t>::value,"grid cardinality API");
static_assert(std::is_same<decltype(std::declval<PDE::Grid1D const&>().dx()),double>::value &&
              std::is_same<decltype(std::declval<PDE::Grid1D const&>().dt()),double>::value &&
              std::is_same<decltype(std::declval<PDE::Grid1D const&>().x(0)),double>::value &&
              std::is_same<decltype(std::declval<PDE::Grid1D const&>().t(0)),double>::value,"grid coordinate API");
static_assert(std::is_same<PDE::Array1D::iterator,std::vector<double>::iterator>::value &&
              std::is_same<PDE::Array1D::const_iterator,std::vector<double>::const_iterator>::value,"iterator aliases");
static_assert(std::is_same<decltype(std::declval<PDE::Array1D&>()[0]), double&>::value, "mutable subscript API");
static_assert(std::is_same<decltype(std::declval<PDE::Array1D const&>()[0]), double const&>::value, "const subscript API");
static_assert(std::is_same<decltype(std::declval<PDE::Array1D&>().at(0)),double&>::value &&
              std::is_same<decltype(std::declval<PDE::Array1D const&>().at(0)),double const&>::value,"checked access API");
static_assert(std::is_same<decltype(std::declval<PDE::Array1D&>().data()),double*>::value &&
              std::is_same<decltype(std::declval<PDE::Array1D const&>().data()),double const*>::value,"pointer API");
static_assert(std::is_same<decltype(std::declval<PDE::Array1D const&>().size()),std::size_t>::value,"size API");
static_assert(std::is_same<decltype(std::declval<PDE::Array1D&>().begin()),PDE::Array1D::iterator>::value &&
              std::is_same<decltype(std::declval<PDE::Array1D&>().end()),PDE::Array1D::iterator>::value &&
              std::is_same<decltype(std::declval<PDE::Array1D const&>().begin()),PDE::Array1D::const_iterator>::value &&
              std::is_same<decltype(std::declval<PDE::Array1D const&>().end()),PDE::Array1D::const_iterator>::value,"iterator API");
static_assert(std::is_same<decltype(std::declval<Scheme const&>().solve()), PDE::Array1D>::value, "const solve API");
static_assert(std::is_same<decltype(std::declval<Scheme const&>().theta()),double>::value,"theta API");
static_assert(PDE::BoundaryConfig1D::Left == 0 && PDE::BoundaryConfig1D::Right == 1, "boundary indices");

TEST(AcceptancePublic, AC001_ArrayStorageAndValueSemantics) {
    PDE::Array1D empty;
    EXPECT_EQ(empty.size(), 0u); EXPECT_EQ(empty.data(), nullptr);
    EXPECT_EQ(empty.begin(), empty.end());
    PDE::Array1D const& ce = empty;
    EXPECT_EQ(ce.data(), nullptr); EXPECT_EQ(ce.begin(), ce.end());
    PDE::Array1D zero(0, 5);
    EXPECT_EQ(zero.size(), 0u); EXPECT_EQ(zero.data(), nullptr);
    PDE::Array1D single(1);
    EXPECT_DOUBLE_EQ(single[0], 0); single.at(0) = 7;
    EXPECT_DOUBLE_EQ(static_cast<PDE::Array1D const&>(single).at(0), 7);
    PDE::Array1D a(3, 2.5);
    EXPECT_EQ(std::distance(a.begin(), a.end()), 3);
    PDE::Array1D const& ca = a;
    EXPECT_EQ(std::distance(ca.begin(), ca.end()), 3);
    for (std::size_t i = 0; i < a.size(); ++i) {
        EXPECT_DOUBLE_EQ(ca[i], 2.5);
        EXPECT_EQ(&a[i], a.data() + i); EXPECT_EQ(&ca.at(i), ca.data() + i);
    }
    double* pointer = a.data(); double& reference = a[1];
    a.fill(4); EXPECT_EQ(a.data(), pointer); EXPECT_DOUBLE_EQ(reference, 4);
    a[1] = 8; EXPECT_EQ(a.data(), pointer); EXPECT_DOUBLE_EQ(reference, 8);
    PDE::Array1D copy(a); copy[0] = 11;
    EXPECT_DOUBLE_EQ(a[0], 4); EXPECT_NE(copy.data(), a.data());
    PDE::Array1D assigned; assigned = a; assigned[2] = 13;
    EXPECT_DOUBLE_EQ(a[2], 4);
    a = a; EXPECT_EQ(values(a), (std::vector<double>{4, 8, 4}));
    const auto original = bits(a);
    auto* self=&a;
    a = std::move(*self); EXPECT_EQ(bits(a), original);
    PDE::Array1D moved(std::move(copy));
    EXPECT_EQ(copy.size(), 0u); EXPECT_EQ(copy.data(), nullptr); EXPECT_EQ(copy.begin(), copy.end());
    EXPECT_EQ(values(moved), (std::vector<double>{11, 8, 4}));
    PDE::Array1D destination(1, 99); destination = std::move(assigned);
    EXPECT_EQ(assigned.size(), 0u); EXPECT_EQ(assigned.data(), nullptr);
    EXPECT_EQ(values(destination), (std::vector<double>{4, 8, 13}));
    const auto movedPointer = moved.data(); double& follows = moved[1];
    moved.swap(destination);
    EXPECT_EQ(destination.data(), movedPointer); EXPECT_EQ(&follows, &destination[1]);
    EXPECT_DOUBLE_EQ(follows, 8);
    moved.swap(moved); EXPECT_EQ(values(moved), (std::vector<double>{4, 8, 13}));
    PDE::Array1D nonfinite(3);
    nonfinite[0] = std::numeric_limits<double>::quiet_NaN();
    nonfinite[1] = std::numeric_limits<double>::infinity();
    nonfinite[2] = -std::numeric_limits<double>::infinity();
    EXPECT_TRUE(std::isnan(nonfinite[0])); EXPECT_TRUE(std::isinf(nonfinite[1]));
}

TEST(AcceptancePublic, AC002_ArrayBoundsHaveNoMutation) {
    for (std::size_t count : {0u, 1u, 3u}) {
        PDE::Array1D a(count, 9); PDE::Array1D const& c = a;
        const auto before = bits(a);
        runtimeFailure([&] { (void)a[count]; });
        runtimeFailure([&] { (void)c[count]; });
        runtimeFailure([&] { (void)a.at(count); });
        runtimeFailure([&] { (void)c.at(count); });
        runtimeFailure([&] { (void)a[std::numeric_limits<std::size_t>::max()]; });
        EXPECT_EQ(bits(a), before);
    }
}

TEST(AcceptancePublic, AC003_GridCoordinatesOwnershipAndValues) {
    PDE::Domain1D d(-2, 3, 0.3, 0.9);
    PDE::Grid1D grid(d, 11, 4);
    EXPECT_EQ(grid.nx(), 11u); EXPECT_EQ(grid.nt(), 4u);
    EXPECT_DOUBLE_EQ(grid.dx(), 0.5); EXPECT_DOUBLE_EQ(grid.dt(), (0.9 - 0.3) / 4);
    EXPECT_DOUBLE_EQ(grid.x(0), -2); EXPECT_DOUBLE_EQ(grid.x(10), 3);
    EXPECT_DOUBLE_EQ(grid.t(0), 0.3); EXPECT_DOUBLE_EQ(grid.t(4), 0.9);
    for (std::size_t i = 1; i < 10; ++i) EXPECT_DOUBLE_EQ(grid.x(i), -2 + double(i) * grid.dx());
    for (std::size_t k = 1; k < 4; ++k) EXPECT_DOUBLE_EQ(grid.t(k), 0.3 + double(k) * grid.dt());
    EXPECT_NE(&grid.domain(), &d);
    const auto* reference = &grid.domain();
    d.xL = 100; d.xR = 101; d.t0 = 200; d.t1 = 201;
    EXPECT_EQ(&grid.domain(), reference); EXPECT_DOUBLE_EQ(grid.domain().xL, -2);
    PDE::Grid1D copy(grid); PDE::Grid1D moved(std::move(copy));
    EXPECT_DOUBLE_EQ(moved.x(10), 3); EXPECT_DOUBLE_EQ(moved.t(4), 0.9);
    PDE::Grid1D detached = [] {
        PDE::Domain1D local(0, 1, 1, 2); return PDE::Grid1D(local, 3, 1);
    }();
    EXPECT_DOUBLE_EQ(detached.domain().t1, 2);
    detached = grid; EXPECT_DOUBLE_EQ(detached.x(10), 3);
    detached = std::move(moved); EXPECT_DOUBLE_EQ(detached.t(4), 0.9);
    runtimeFailure([&] { (void)grid.x(11); }); runtimeFailure([&] { (void)grid.t(5); });
    runtimeFailure([&] { (void)grid.x(std::numeric_limits<std::size_t>::max()); });
    // nt counts intervals; nt+1 need not be representable or allocated.
    PDE::Grid1D manyTimes(PDE::Domain1D(0, 1, 0, 1), 3, std::numeric_limits<std::size_t>::max());
    EXPECT_DOUBLE_EQ(manyTimes.t(manyTimes.nt()), 1);
}

TEST(AcceptancePublic, AC004_DomainAndGridRejectInvalidInputs) {
    const double maximum = std::numeric_limits<double>::max();
    const double tiny = std::numeric_limits<double>::denorm_min();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    for (auto tuple : {std::array<double,4>{0,0,0,1}, {1,0,0,1}, {0,1,0,0}, {0,1,1,0},
                       {-maximum,maximum,0,1}, {0,1,-maximum,maximum}})
        runtimeFailure([&] { PDE::Domain1D d(tuple[0], tuple[1], tuple[2], tuple[3]); });
    for (std::size_t field = 0; field < 4; ++field) for (double invalid : {nan, infinity, -infinity}) {
        std::array<double,4> v{{0,1,0,1}}; v[field] = invalid;
        runtimeFailure([&] { PDE::Domain1D d(v[0],v[1],v[2],v[3]); });
        PDE::Domain1D changed(0,1,0,1);
        std::array<double*,4> fields{{&changed.xL,&changed.xR,&changed.t0,&changed.t1}};
        *fields[field] = invalid;
        runtimeFailure([&] { PDE::Grid1D g(changed,3,1); });
    }
    PDE::Domain1D valid(0,1,0,1);
    for (std::size_t nx : {0u,1u,2u}) runtimeFailure([&] { PDE::Grid1D g(valid,nx,1); });
    runtimeFailure([&] { PDE::Grid1D g(valid,3,0); });
    const std::size_t maxElements = std::vector<double>().max_size();
    if (maxElements < std::numeric_limits<std::size_t>::max())
        runtimeFailure([&] { PDE::Grid1D g(valid,maxElements + 1,1); });
    PDE::Domain1D tinySpace(0,tiny,0,1), tinyTime(0,1,0,tiny);
    runtimeFailure([&] { PDE::Grid1D g(tinySpace,3,1); });
    runtimeFailure([&] { PDE::Grid1D g(tinyTime,3,2); });
    for (auto tuple : {std::array<double,4>{0,0,0,1}, {1,0,0,1}, {0,1,0,0}, {0,1,1,0},
                       {-maximum,maximum,0,1}, {0,1,-maximum,maximum}}) {
        PDE::Domain1D changed(0,1,0,1);
        changed.xL=tuple[0]; changed.xR=tuple[1]; changed.t0=tuple[2]; changed.t1=tuple[3];
        runtimeFailure([&] { PDE::Grid1D g(changed,3,1); });
    }
}

TEST(AcceptancePublic, AC005_DefaultsConstructionAndTheta) {
    PDE::PDE1D p;
    ASSERT_TRUE(p.diffusion); ASSERT_TRUE(p.advection); ASSERT_TRUE(p.reaction); ASSERT_TRUE(p.source);
    EXPECT_FALSE(p.initial); EXPECT_FALSE(p.boundaryL); EXPECT_FALSE(p.boundaryR);
    EXPECT_DOUBLE_EQ(p.diffusion(-1,2), 1); EXPECT_DOUBLE_EQ(p.advection(-1,2), 0);
    EXPECT_DOUBLE_EQ(p.reaction(-1,2), 0); EXPECT_DOUBLE_EQ(p.source(-1,2), 0);
    for (auto side : {PDE::BoundaryConfig1D::Left,PDE::BoundaryConfig1D::Right})
        EXPECT_EQ(p.boundaryConfig.types[side], PDE::BoundaryType::Dirichlet);
    PDE::Grid1D grid(PDE::Domain1D(0,1,0,0.1),5,2);
    p.boundaryConfig.types[0] = PDE::BoundaryType::Neumann;
    Scheme solver(grid,p); auto factory = PDE::makeSolver1D(grid,p);
    EXPECT_DOUBLE_EQ(solver.theta(),0.5); EXPECT_DOUBLE_EQ(factory.theta(),0.5);
    for (double theta : {0.5,0.75,1.0,std::nextafter(0.5,1.0)}) {
        solver.setTheta(theta); EXPECT_DOUBLE_EQ(solver.theta(),theta);
        for (double bad : {0.49,1.01,std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}) {
            runtimeFailure([&] { solver.setTheta(bad); }); EXPECT_DOUBLE_EQ(solver.theta(),theta);
        }
    }
    Scheme copy(solver), moved(std::move(copy)); EXPECT_DOUBLE_EQ(moved.theta(),solver.theta());
    int calls = 0;
    for (auto role : roles()) p.*role = [&](double,double) -> double { ++calls; throw UserFailure{"construction"}; };
    Scheme noSampling(grid,p); auto noFactorySampling = PDE::makeSolver1D(grid,p);
    EXPECT_EQ(calls,0); (void)noSampling; (void)noFactorySampling;
}
} // namespace
