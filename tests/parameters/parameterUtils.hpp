#pragma once

#include "powerflow/PowerFlowSolver.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

/**
 * @brief Default tolerance threshold for parameter variation.
 */
const double VARIATION = 1e-4;

/**
 * @brief Tolerance threshold for impedance comparison in solveParams.
 */
const double IMPEDANCE_SOLVE_TOLERANCE = 1e-2;

/**
 * @brief Tolerance threshold for impedance comparison in solveParamsReg.
 */
const double IMPEDANCE_REG_TOLERANCE = 1.1e-2;

enum class ExpectationType
{
    Contains,       // The log MUST contain the expectedError string.
    DoesNotContain, // The log MUST NOT contain the expectedError string.
};

struct ExpectedError
{
    std::string message;
    ExpectationType type;
};

struct ParameterRegTestConfig
{
    /** @brief Path to the truthful network file. */
    std::string referenceNetworkPath;

    /** @brief Path to the network file containing the parameter anomaly. */
    std::string distortedNetworkPath;

    /** @brief The measured values for each node in the network. */
    std::unordered_map<node_idx_t, MeasuredValues> measuredValues;

    /** @brief The voltage for every node in the network. */
    std::vector<complex_t> slackVoltages;

    /** @brief The threshold for the parameter estimation. */
    double threshold;

    /** @brief The maximum number of iterations for the parameter estimation. */
    int maxIterations;
    
    /** @brief The expected error message to look for in the logs. */
    ExpectedError error{};
};

/**
 * @brief Configuration for executing a parameter anomaly validation test.
 */
struct ParameterTestConfig
{
    /** @brief Path to the truthful network file. */
    std::string referenceNetworkPath;

    /** @brief Path to the network file containing the parameter anomaly. */
    std::string distortedNetworkPath;

    /** @brief The power for every load node in the network. */
    std::vector<complex_t> S;

    /** @brief The voltage for every node in the network. */
    std::vector<complex_t> V;

    /** @brief The expected error message to look for in the logs. */
    ExpectedError error{};
};

/**
 * @brief Loads a Network object from a file.
 * Reads the power network configuration from the specified path. Throws an exception
 * if the file cannot be opened or parsed.
 *
 * @param path The filesystem path to the network configuration file.
 *
 * @return std::shared_ptr<Network> A shared pointer to the loaded Network object.
 *
 * @throws std::runtime_error If the file fails to open.
 */
std::shared_ptr<Network> loadNetwork(const std::string &path);

/**
 * @brief Computes the absolute node voltages for the given network.
 *
 * @param network Pointer to the network under test.
 * @param S Powers for each load node in the network.
 * @param V Voltage for each node in the network.
 *
 * @return std::unordered_map<node_idx_t, complex_t> A map of load node indices to their complex voltages.
 */
std::unordered_map<node_idx_t, complex_t> getAbsoluteVoltages(
    std::shared_ptr<Network> network,
    const std::vector<complex_t> &S,
    const std::vector<complex_t> &V //
);

/**
 * @brief Executes a parameter anomaly validation test case.
 *
 * @param config The configuration parameters for the test scenario.
 */
void validateNetworkImpedances(const ParameterTestConfig &config);

/**
 * @brief Runs the complete solver workflow and validates that all edge
 * impedances match exactly between two network files.
 *
 * @param config The configuration parameters for the test scenario.
 */
void checkNetworkImpedancesAreEqual(const ParameterTestConfig &config);

/**
 * @brief Runs the solveParamsReg workflow and validates that the estimated
 * impedances match the nominal ones within a tolerance.
 *
 * @param config The configuration parameters for the test scenario.
 */
void checkNetworkImpedancesReg(const ParameterRegTestConfig &config);

/**
 * @brief Runs the solveParamsReg workflow and validates that the correct error
 * is logged to std::cerr.
 *
 * @param config The configuration parameters for the test scenario.
 */
void validateNetworkImpedancesRegError(const ParameterRegTestConfig &config);