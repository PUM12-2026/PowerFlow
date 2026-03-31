#ifndef POWERFLOW_GAUSS_SEIDEL_SOLVER_H
#define POWERFLOW_GAUSS_SEIDEL_SOLVER_H

#include "powerflow/solvers/GridSolver.hpp"

// GridSolver implementing the Gauss-Seidel algorithm.
class GaussSeidelSolver : public GridSolver
{
public:
    GaussSeidelSolver(Grid* grid, Logger* const logger, int maxIter, double precision);
    int solve() override;
    void reset() override;

private:
    std::vector<complex_t> y{};    // Admittance vector
    bool firstRun = true; // Used to indicate that slack powers have not been calculated

    // Checks if the solution has converged.
    bool hasConverged();
};

#endif
