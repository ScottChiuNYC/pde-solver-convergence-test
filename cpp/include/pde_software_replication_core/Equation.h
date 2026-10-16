#ifndef PDE_SOFTWARE_REPLICATION_CORE_EQUATION_H
#define PDE_SOFTWARE_REPLICATION_CORE_EQUATION_H
#include "Types.h"
namespace PDE {
struct PDE1D {
    PDE1D()
        : diffusion([](double, double) { return 1.0; }),
          advection([](double, double) { return 0.0; }),
          source([](double, double) { return 0.0; }),
          reaction([](double, double) { return 0.0; }) {}
    Func::F1D diffusion, advection, source, reaction;
    Func::F1D initial, boundaryL, boundaryR;
    BoundaryConfig1D boundaryConfig;
};
} // namespace PDE
#endif
