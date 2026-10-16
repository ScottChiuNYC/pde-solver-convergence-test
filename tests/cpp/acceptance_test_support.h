#ifndef C2C_ACCEPTANCE_TEST_SUPPORT_H
#define C2C_ACCEPTANCE_TEST_SUPPORT_H
#include <pde_software_replication_core/PDE.h>
#include <gtest/gtest.h>
#include "acceptance_reference.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <new>
#include <string>
#include <vector>

namespace acceptance {
inline PDE::Array1D array(std::initializer_list<double> values) {
    PDE::Array1D out(values.size());
    std::copy(values.begin(), values.end(), out.begin());
    return out;
}
inline PDE::Array1D array(std::vector<double> const& values) {
    PDE::Array1D out(values.size());
    std::copy(values.begin(), values.end(), out.begin());
    return out;
}
inline std::vector<double> values(PDE::Array1D const& a) {
    return std::vector<double>(a.begin(), a.end());
}
inline std::vector<std::uint64_t> bits(PDE::Array1D const& a) {
    static_assert(sizeof(double) == sizeof(std::uint64_t), "binary64 test platform");
    std::vector<std::uint64_t> out(a.size());
    for (std::size_t i = 0; i < a.size(); ++i) std::memcpy(&out[i], &a[i], sizeof(double));
    return out;
}
template<class Operation> inline std::string runtimeFailure(Operation operation) {
    try { operation(); }
    catch (std::runtime_error const& error) {
        EXPECT_FALSE(std::string(error.what()).empty()) << "numerical/configuration error needs diagnostic context";
        return error.what();
    }
    catch (std::exception const& error) {
        ADD_FAILURE() << "wrong exception class: " << error.what();
        return {};
    }
    catch (...) { ADD_FAILURE() << "wrong exception type"; return {}; }
    ADD_FAILURE() << "expected std::runtime_error";
    return {};
}
inline PDE::PDE1D constantData(double value = 1) {
    PDE::PDE1D p;
    p.initial = [=](double, double) { return value; };
    p.boundaryL = [=](double, double) { return value; };
    p.boundaryR = [=](double, double) { return value; };
    return p;
}
inline PDE::PDE1D data(acceptance_reference::Data const& d) {
    PDE::PDE1D p;
    p.diffusion = d.a; p.advection = d.b; p.reaction = d.c; p.source = d.f;
    p.initial = d.initial; p.boundaryL = d.left; p.boundaryR = d.right;
    return p;
}
inline void near(PDE::Array1D const& actual, std::vector<double> const& expected, double tolerance) {
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        ASSERT_TRUE(std::isfinite(actual[i])) << "node " << i;
        EXPECT_NEAR(actual[i], expected[i], tolerance) << "node " << i;
    }
}
using Member = PDE::Func::F1D PDE::PDE1D::*;
inline std::array<Member, 7> roles() {
    return {{&PDE::PDE1D::diffusion, &PDE::PDE1D::advection, &PDE::PDE1D::reaction,
             &PDE::PDE1D::source, &PDE::PDE1D::initial, &PDE::PDE1D::boundaryL,
             &PDE::PDE1D::boundaryR}};
}
inline std::array<char const*, 7> roleNames() {
    return {{"diffusion", "advection", "reaction", "source", "initial", "boundaryL", "boundaryR"}};
}
struct UserFailure { std::string payload; };
struct UserStdFailure : std::runtime_error {
    explicit UserStdFailure(std::string payload) : std::runtime_error(payload), marker(314159) {}
    int marker;
};
} // namespace acceptance
#endif
