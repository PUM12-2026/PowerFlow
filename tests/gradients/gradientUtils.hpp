#pragma once

#include "powerflow/logger/CppLogger.hpp"
#include "powerflow/PowerFlowSolver.hpp"
#include "powerflow/NetworkLoader.hpp"
#include "../tests/catch.hpp"

#include <iostream>
#include <fstream>
#include <complex>
#include <vector>
#include <queue>

const node_idx_t ROOT_NODE_ID = 0;

const double DELTA = 1e-10;
const double PRECISION = 1e-12;
const double TEST_TOLERANCE = 10e-6;
const int MAX_ITERATIONS = 10000;

/**
 * @brief Represents the electrical state of all nodes at a specific point in time.
 */
struct NodeState
{
    std::vector<complex_t> v;
    std::vector<complex_t> s;
    std::vector<complex_t> i;
    std::vector<complex_t> sloss;
};

/**
 * @brief Container for gradients computed numerically.
 */
struct NumericGradients
{
    std::vector<std::pair<double, double>> dVdS;
    std::vector<std::pair<double, double>> dSdS;
    std::vector<std::pair<double, double>> dIdS;
    std::vector<std::pair<double, double>> dSlossdS;
};

/**
 * @brief Container for the test environment.
 */
struct GradientTestEnvironment
{
    std::shared_ptr<Network> network;
    std::unique_ptr<PowerFlowSolver> pfs;
};

/**
 * @brief Loads the provided network, initializes the solver, and sets up the test environment.
 *
 * @param filename The path to the network file to load.
 * @param logger The logger to use for logging (default: nullptr).
 * @param computeGradients Whether to enable gradient computation in the solver settings (default: true).
 */
GradientTestEnvironment setupEnvironment(const std::string &filename, Logger *logger = nullptr, bool computeGradients = true);

/**
 * @brief Extracts all leaf (LOAD) nodes from the entire network.
 *
 * @param network The network to extract leaf nodes from.
 *
 * @return A vector of pairs, where each pair contains the grid index and node index of a leaf node in the network.
 */
std::vector<std::pair<size_t, node_idx_t>> getNetworkLeafNodeIndices(const Network &network);

/**
 * @brief Traverses the radial grid to determine which edge is upstream for each node.
 *
 * @param grid The grid to analyze for upstream edge mapping.
 *
 * @return A vector where the index corresponds to a node index and the value is the index of the upstream edge.
 */
std::vector<edge_idx_t> mapUpstreamEdges(const Grid &grid);

/**
 * @brief Captures the current electrical state (V, S, I, and Sloss) of all nodes in the grid.
 *
 * @param grid The grid for which to capture the current state.
 *
 * @return A NodeState struct containing the voltage, power, current, and power loss for each node in the grid.
 */
NodeState captureCurrentNetworkState(const Grid &grid);

/**
 * @brief Computes a gradient matrix entry using the central difference numerical method.
 *
 * @param plusP The state of the network when the leaf node's real power is increased by DELTA.
 * @param minusP The state of the network when the leaf node's real power is decreased by DELTA.
 * @param plusQ The state of the network when the leaf node's complex power is increased by DELTA.
 * @param minusQ The state of the network when the leaf node's complex power is decreased by DELTA.
 *
 * @return A pair of doubles representing the real and imaginary parts of the gradient computed using the central difference method.
 */
std::pair<double, double> calculateCentralDifference(
    const complex_t &plusP, const complex_t &minusP,
    const complex_t &plusQ, const complex_t &minusQ //
);

/**
 * @brief Performs numerical sensitivity analysis to compute gradients for each leaf node in the network.
 *
 * @param solver The power flow solver to use for solving the network after delta modifications.
 * @param grid The grid for which to compute the numeric gradients.
 * @param S The original power injection vector for the leaf nodes.
 * @param V The original voltage vector for the leaf nodes.
 * @param leafIdx The index of the leaf node for which to compute the gradients.
 *
 * @return A NumericGradients struct containing the numerically computed gradients (dVdS, dSdS, dIdS, and dSlossdS) for the specified leaf node.
 */
NumericGradients computeNumericGradients(
    PowerFlowSolver &solver,
    const Grid &grid,
    std::vector<complex_t> &S,
    std::vector<complex_t> &V,
    size_t leafIdx //
);

/**
 * @brief Validates the solver's analytic gradients against the numeric central difference approximations.
 *
 * @param index The index of the node for which the gradient is being verified.
 * @param gridIdx The index of the grid to which the node belongs.
 * @param leafGridIdx The index of the grid to which the leaf node belongs.
 * @param leafNodeIdx The index of the leaf node for which the gradient is being verified
 * @param analytic The analytic gradient matrix entry calculated by the solver for the specified node and leaf node.
 * @param numeric The numeric gradient approximation calculated using the central difference method for the specified node.
 * @param label A string label indicating which gradient type is being verified (e.g., "dVdS", "dSdS", etc.) for logging purposes.
 * @param isEdge A boolean flag indicating whether the gradient being verified is for an edge (true) or a node (false), used for logging purposes.
 */
void verifyAnalyticGradients(
    int index,
    size_t gridIdx,
    size_t leafGridIdx,
    size_t leafNodeIdx,
    const Eigen::Matrix2d &analytic,
    std::pair<double, double> numeric,
    const char *label,
    bool isEdge = false //
);

/**
 * @brief Runs the gradient tests for a specific grid, comparing analytic gradients with numeric approximations for all leaf nodes.
 *
 * @param environment The test environment containing the network and solver.
 * @param gridIdx The index of the grid for which to run the gradient tests.
 * @param S The original power injection vector for the leaf nodes.
 * @param V The original voltage vector for the leaf nodes.
 */
void runGradientTests(
    const GradientTestEnvironment &environment,
    size_t gridIdx,
    std::vector<complex_t> S,
    std::vector<complex_t> V //
);