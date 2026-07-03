#ifndef POWERFLOW_POWER_FLOW_SOLVER_H
#define POWERFLOW_POWER_FLOW_SOLVER_H

#include "powerflow/network.hpp"
#include "powerflow/solvers/GridSolver.hpp"
#include "powerflow/logger/Logger.hpp"
#include "powerflow/ParameterValidator.hpp"
#include "powerflow/solverSettings.hpp"
#include <memory>
#include <tuple>
#include <vector>
#include <string>
#include <unordered_map>

// Class responsible for solving an entire Network.
class PowerFlowSolver
{
public:
    // network - The network to solve.
    // logger - Logger object.
    PowerFlowSolver(std::shared_ptr<Network> network, SolverSettings settings, Logger *const logger);

    // Solve network.
    void solve(const std::vector<complex_t> &P, const std::vector<complex_t> &V);

    void solveById(const std::unordered_map<node_key_t, complex_t> &S, const std::unordered_map<node_key_t, complex_t> &V);

    /** 
     * @brief Identifies invalid parameters and adjusts them. 
     * 
     * WARNING: This solver is not recommended. It only detects invalid parameters in some
     * cases. In testing it only detects invalid parameters when the true impedance differs 
     * from the nominal impedance by at least 60%. 
     * 
     * Use solveParamsReg instead.
     */ 
    void solveParams(const std::unordered_map<node_idx_t, complex_t> &V, double precision);

    /**  
     * Implementation of inverse BFS
     * https://ietresearch.onlinelibrary.wiley.com/doi/10.1049/stg2.12177.
     * Assumes all LOAD nodes are leaf nodes, and vice versa.
     * Assumes the grid is radial.
     * Assumes low power losses (S_loss) in cables.
     * Assumes there is one grid.
     * 
     * NOTE: Will update impedances in grid, even if the algorithm fails to converge.
     * 
     * @param measuredValues Map of LOAD node IDs to time-series voltages and power injections
     * @param slackVoltages Time-series voltages at SLACK node
     */
    std::vector<complex_t> solveParamsOLS(std::unordered_map<node_key_t, MeasuredValues> &measuredValues, 
        std::vector<complex_t> &slackVoltages);

    std::vector<complex_t> solveParamsLAD(std::unordered_map<node_key_t, MeasuredValues> &measuredValues, 
        std::vector<complex_t> &slackVoltages);
    
	// Returns all LOAD voltages in the network.
	std::vector<complex_t> getLoadVoltages() const;

    // Returns all voltages in the network.
    std::vector<complex_t> getAllVoltages() const;

    // Returns all currents in the network.
    std::vector<complex_t> getCurrents() const;

    // Returns all SLACK_IMPLICIT/SLACK powers in the network.
    std::vector<complex_t> getSlackPowers() const;

    // Returns all cable impedances in the network.
    std::vector<complex_t> getImpedances() const;

    // Sets all cable impedances in the network;
    void setImpedances(std::vector<complex_t> &Z);

    // Returns the gradients of all nodes in the network except SLACK (root) node, with respect to all of the LOAD powers.
    std::vector<std::vector<std::array<double, 2>>> getDvDs() const;

    // Returns the gradients of all cables in the network, with respect to all of the LOAD powers.
    std::vector<std::vector<std::array<double, 2>>> getDiDs() const;

    // Returns the gradients of all nodes in the network except the LOAD nodes, with respect to all of the LOAD powers.
    std::vector<std::vector<std::array<double, 2>>> getDsDs() const;

    // Returns the gradients of all cables in the network, with respect to all of the LOAD powers.
    std::vector<std::vector<std::array<double, 2>>> getDslossDs() const;

    // Resets powers to 0 and voltages to 1.
    void reset();

    // Saves network to file
    void save(std::ofstream& file);

    /**
     * Returns true if the network is radial, else false.
     */
    bool isRadial();

    /**
     * Simplifies the network by merging cables in series
     */
    void simplifyNetwork();

private:
    std::vector<std::unique_ptr<GridSolver>> gridSolvers;
    std::shared_ptr<Network> network;
    SolverSettings settings;
    bool firstRun{true};
    Logger *const logger{nullptr};

    // Creates appropriate GridSolvers for each grid in the network.
    void createGridSolvers();

    // Updates LOAD node powers.
    void updateLoads(const std::vector<complex_t> &P);

    // Updates SLACK node voltages.
    void updateExternalVoltages(const std::vector<complex_t> &V);

    // Runs the GridSolvers and combines the result.
    void runGridSolvers();
};

#endif
