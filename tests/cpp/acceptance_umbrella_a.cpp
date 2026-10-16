#include <pde_software_replication_core/PDE.h>
#include <pde_software_replication_core/PDE.h>
#include <cmath>
#include <vector>

// A separate translation unit uses the public Section 6 example without
// relying on another include to supply public declarations.
std::vector<double> acceptanceUmbrellaOtherTranslationUnit() {
    const double pi=std::acos(-1.0);
    PDE::Domain1D domain(0,1,0,0.1);
    PDE::Grid1D grid(domain,101,1000);
    PDE::PDE1D pde;
    pde.initial=[pi](double,double x) { return std::sin(pi*x); };
    pde.boundaryL=[](double,double) { return 0.0; };
    pde.boundaryR=[](double,double) { return 0.0; };
    auto solver=PDE::makeSolver1D(grid,pde);
    PDE::Array1D solution=solver.solve();
    return std::vector<double>(solution.begin(),solution.end());
}
