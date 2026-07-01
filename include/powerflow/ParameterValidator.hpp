#pragma once

#include "powerflow/network.hpp"
#include "powerflow/logger/Logger.hpp"
#include "powerflow/solverSettings.hpp"
#include <unordered_map>
#include <Eigen/Dense>


/**
 * Container for time-series voltages and power injections at a node.
 * Required by regression-based parameter estimation
 */
struct MeasuredValues
{
    // Voltages over time at node
    std::vector<complex_t> U;
    // Power injections over time at node
    std::vector<complex_t> S;
};

/**
 * Class for handling cable parameter estimation.
 * 
 * There are two main methods for estimation, the first one is used when calling validate. It is not recommended.
 * 
 * The second one relies on regression and is used when calling validateRegression.
 * 
 * See PowerFlowSolver for more details.
 */
class ParameterValidator
{
public:
    ParameterValidator(Grid* grid, Logger* const logger, SolverSettings* const settings, const std::unordered_map<node_idx_t, complex_t> &measuredV, 
        double precision);

    /**
     * Validate and estimate cable parameters.
     * This method is not recommended, use validateRegression instead.
     */
    void validate();

    /**  
     * Implementation of inverse BFS
     * https://ietresearch.onlinelibrary.wiley.com/doi/10.1049/stg2.12177.
     * Assumes all LOAD nodes are leaf nodes, and vice versa.
     * Assumes the grid is radial.
     * Assumes low power losses (S_loss) in cables.
     * Assumes there is one grid.
     * 
     * @param measuredValues Map of LOAD node IDs to time-series voltages and power injections
     * @param slackVoltages Time-series voltages at SLACK node
     * @param convergenceThreshold Minimum change in impedances between iterations before convergence is accepted
     * @param maxIterations Max amount of iterations
     */
    std::vector<complex_t> validateRegression(std::unordered_map<node_idx_t, MeasuredValues> &measuredValues, 
        std::vector<complex_t> &slackVoltages);

    std::vector<complex_t> validateLAD(std::unordered_map<node_idx_t, MeasuredValues> &measuredValues, 
        std::vector<complex_t> &slackVoltages);

private:
    double precision;
    Grid* grid{nullptr};
    Logger* const logger{nullptr};
    SolverSettings* settings{nullptr};
    // Map of node IDs to measured (true) voltages
    std::unordered_map<node_idx_t, complex_t> const measuredV;
    std::unordered_map<node_idx_t, MeasuredValues> measuredValues;

    /**
     * If true, validateRegression will only estimate resistances, 
     * else both resistances and reactances will be estimated.
     * True when all Q measurements are zero.
     */
    bool resistanceOnly = false;

    /**
     * If true, will use NNLS instead of OLS. Managed internally by validateRegression.
     */
    bool useNNLS = false;

    /** Recursive function that traverses all child nodes of node with given ID. 
     * Returns a tuple of current I, power injection S, voltage U, and validity. 
     * If invalid parameters are detected, validity is false. */
    std::tuple<complex_t, complex_t, complex_t, bool> validateRecursive(node_idx_t node_id);

    complex_t BackwardSweep(node_idx_t n, size_t t, std::vector<complex_t> &branchCurrents);

    /**
     * Finds path from n to m, assuming m is downstream of n. 
     * If no path is found, returns false. 
     * If path is found, returns true, and places path into vector.
     */
    bool FindPath(node_idx_t n, node_idx_t m, std::vector<edge_idx_t> &path);

    /**
     * Given branch currents for whole grid, and slack voltages, peforms
     * OLS regression to estimate cable parameters for whole grid.
     * Places estimated parameters in the newImpedances vector.
     */
    void EstimateParameters(std::vector<std::vector<complex_t>> &branchCurrents, std::vector<complex_t> &slackVoltages, 
        std::vector<complex_t> &newImpedances);

    /**
     * Recursively updates voltages in grid, starting from node n.
     */
    void ForwardSweep(node_idx_t n, size_t t, std::vector<complex_t> &branchCurrents, 
        complex_t parentVoltage, edge_idx_t parentEdge);

    /** 
     * Finds all loads and edges downstream of node n, and places them into
     * loads vector and edges vector, respectively.
     */
    void GetDownStream(node_idx_t n, std::vector<edge_idx_t> &edges, std::vector<node_idx_t> &loads);

    /**
     * Solves LAD problem. Places estimated impedances in Z vector.
     */
    void SolveLAD(const Eigen::MatrixXd &A, const Eigen::VectorXd &b, std::vector<double> &Z);

    void EstimateParametersLAD(std::vector<std::vector<complex_t>> &branchCurrents, std::vector<complex_t> &slackVoltages, 
        std::vector<complex_t> &newImpedances);
};