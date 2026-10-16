#include <pde_software_replication_core/hello.h>
#include <pde_software_replication_core/PDE.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>

int main() {
    const auto message = hello();
    if (message != "Hello, World!") return 1;
    std::cout << message << '\n';
    PDE::Grid1D grid(PDE::Domain1D(0, 2, 0, 1), 3, 1);
    PDE::PDE1D pde;
    pde.diffusion = [](double t, double) { return 1 + t; };
    pde.advection = [](double t, double) { return 2 * t; };
    pde.reaction = [](double t, double) { return -2 * t; };
    pde.source = [](double t, double) { return 2 + 4 * t; };
    pde.initial = [](double, double x) {
        if (x != 1) throw std::runtime_error("initial endpoint unexpectedly sampled");
        return 2.0;
    };
    pde.boundaryL = [](double t, double) { return 1 + 2 * t; };
    pde.boundaryR = [](double t, double) { return 5 - t; };
    auto solver = PDE::makeSolver1D(grid, pde);
    for (double theta : {0.5, 0.75, 1.0}) {
        solver.setTheta(theta);
        const auto solution = solver.solve();
        const double expected = (7 + 12 * theta) / (1 + 4 * theta);
        if (solution.size() != 3 || solution[0] != 3 || solution[2] != 4 ||
            !std::isfinite(solution[1]) || std::abs(solution[1] - expected) > 1e-12) return 2;
        std::cout << "Installed solver theta=" << theta << " interior=" << solution[1] << '\n';
    }
    const double pi = std::acos(-1.0);
    PDE::Grid1D heatGrid(PDE::Domain1D(0, 1, 0, 0.1), 101, 1000);
    PDE::PDE1D heat;
    heat.initial = [pi](double, double x) { return std::sin(pi * x); };
    heat.boundaryL = heat.boundaryR = [](double, double) { return 0.0; };
    const auto heatSolution = PDE::makeSolver1D(heatGrid, heat).solve();
    if (heatSolution.size() != 101 || heatSolution[0] != 0 || heatSolution[100] != 0) return 3;
    double maxError = 0, squareError = 0;
    for (std::size_t i = 0; i < heatSolution.size(); ++i) {
        const double error = heatSolution[i] - std::exp(-pi*pi*0.1) * std::sin(pi*heatGrid.x(i));
        if (!std::isfinite(error)) return 4;
        maxError = std::max(maxError, std::abs(error)); squareError += error * error;
    }
    const double l2Error = std::sqrt(heatGrid.dx() * squareError);
    std::cout << "Installed heat solver E_inf=" << maxError << " E_2=" << l2Error << '\n';
    return maxError <= 4e-5 && l2Error <= 3e-5 ? 0 : 5;
}
