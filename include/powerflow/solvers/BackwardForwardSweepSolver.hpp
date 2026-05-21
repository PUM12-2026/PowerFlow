#ifndef POWERFLOW_BACKWARD_FORWARD_SWEEP_H
#define POWERFLOW_BACKWARD_FORWARD_SWEEP_H

#include "powerflow/computers/GradientComputer.hpp"
#include "powerflow/solvers/GridSolver.hpp"
#include "powerflow/network.hpp"

// Default index for the root node.
const node_idx_t DEFAULT_ROOT_INDEX = -1;

// GridSolver implementing the Backward-Forward-Sweep algorithm.
class BackwardForwardSweepSolver : public GridSolver
{
public:
    BackwardForwardSweepSolver(
        Grid *grid,
        Logger *logger,
        int maxIteration,
        double precision,
        bool computeGradients,
        size_t networkLeafCount,
        const std::vector<size_t> &gridLeafIndicies //
    );

    /**
     * @brief Executes the Backward-Forward-Sweep algorithm to solve the power flow problem.
     */
    int solve() override;

    /**
     * @brief Resets the solver's internal state.
     */
    void reset() override;

private:
    // The number of leaf nodes in the whole network, so not only this grid.
    size_t networkLeafCount;

    // Object that computes the gradients.
    GradientComputer gradientComputer;

    // The indices of the leaf nodes.
    std::vector<size_t> gridLeafIndicies;

    // Indicates if the gradient should be computed.
    bool computeGradients;

    // Index of the root node (slack node).
    node_idx_t rootIdx = DEFAULT_ROOT_INDEX;

    // Current vector.
    std::vector<complex_t> I;

    // Used to indicate that slack power has not been computed.
    bool firstRun = true;

    /**
     * @brief Recursive function that performs a single Backward-Forward sweep.
     *
     * @param nodeIdx          Index of current node in the grid.
     * @param prevEdgeIdx      Index of upstream edge (-1 for root node).
     *
     * @return complex_t: The total power flow (P + jQ) upstream of the current branch.
     */
    complex_t sweep(
        node_idx_t nodeIdx,
        edge_idx_t prevEdgeIdx // ,
    );

    /**
     * @brief Checks if the solution has converged by comparing the current power flow with the previous iteration's power flow.
     *
     * @return bool: True if the solution has converged, false otherwise.
     */
    bool hasConverged();
};

#endif
