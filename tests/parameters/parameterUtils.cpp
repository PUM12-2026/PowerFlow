#include "powerflow/logger/CppLogger.hpp"
#include "powerflow/NetworkLoader.hpp"
#include "parameterUtils.hpp"
#include "../catch.hpp"

#include <fstream>

std::shared_ptr<Network> loadNetwork(const std::string &path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Could not open " + path);
    return NetworkLoader(file).loadNetwork();
}

std::unordered_map<node_idx_t, complex_t> getAbsoluteVoltages(
    std::shared_ptr<Network> network,
    const std::vector<complex_t> &S,
    const std::vector<complex_t> &V //
)
{
    std::ostringstream discard;
    CppLogger logger(discard);
    PowerFlowSolver solver(network, {}, &logger);
    solver.solve(S, V);

    std::unordered_map<node_idx_t, complex_t> results;
    auto loadVoltages = solver.getLoadVoltages();
    int loadCount = 0;
    for (size_t i = 0; i < network->grids.at(0).nodes.size(); ++i)
    {
        if (network->grids.at(0).nodes[i].type == NodeType::LOAD)
        {
            results[i] = std::abs(loadVoltages.at(loadCount++));
        }
    }
    return results;
}

void validateNetworkImpedances(const ParameterTestConfig &config)
{
    auto referenceNetwork = loadNetwork(config.referenceNetworkPath);
    auto distortedNetwork = loadNetwork(config.distortedNetworkPath);
    auto referenceVoltages = getAbsoluteVoltages(referenceNetwork, config.S, config.V);

    std::ostringstream logStream;
    CppLogger logger(logStream);
    PowerFlowSolver solver(distortedNetwork, {}, &logger);

    solver.solve(config.S, config.V);
    solver.solveParams(referenceVoltages, VARIATION);

    std::string solverOutput = logStream.str();

    if (config.error.type == ExpectationType::Contains)
    {
        CHECK(solverOutput.find(config.error.message) != std::string::npos);
    }
    else if (config.error.type == ExpectationType::DoesNotContain)
    {
        CHECK(solverOutput.find(config.error.message) == std::string::npos);
    }
}

void checkNetworkImpedancesReg(const ParameterRegTestConfig &config)
{
    auto refNetwork = loadNetwork(config.referenceNetworkPath);
    auto distNetwork = loadNetwork(config.distortedNetworkPath);

    std::ostringstream logStream;
    CppLogger logger(logStream);
    SolverSettings settings{};
    settings.ols_precision = config.threshold;
    settings.max_iterations_ols = config.maxIterations;
    
    // We run parameter estimation on the distorted network
    PowerFlowSolver solver(distNetwork, settings, &logger);
    
    // But the reference impedances come from the reference network
    PowerFlowSolver refSolver(refNetwork, settings, &logger);
    std::vector<complex_t> refImpedances = refSolver.getImpedances();

    auto slackVoltages = config.slackVoltages;
    auto measuredValues = config.measuredValues;
    solver.solveParamsOLS(measuredValues, slackVoltages);

    std::vector<complex_t> estimatedImpedances = solver.getImpedances();

    for (size_t i = 0; i < refImpedances.size(); i++)
    {
        double error = std::abs(estimatedImpedances[i] - refImpedances[i]) / std::abs(refImpedances[i]);
        CHECK(error <= IMPEDANCE_REG_TOLERANCE);
    }
}

void checkNetworkImpedancesAreEqual(const ParameterTestConfig &config)
{
    auto referenceNetwork = loadNetwork(config.referenceNetworkPath);
    auto distortedNetwork = loadNetwork(config.distortedNetworkPath);
    auto referenceVoltages = getAbsoluteVoltages(referenceNetwork, config.S, config.V);

    std::ostringstream logStream;
    CppLogger logger(logStream);
    PowerFlowSolver solver(distortedNetwork, {}, &logger);

    solver.solve(config.S, config.V);
    solver.solveParams(referenceVoltages, VARIATION);

    const auto &referenceEdges = referenceNetwork->grids.at(0).edges;
    const auto &distortedEdges = distortedNetwork->grids.at(0).edges;

    // Check that the impedances of all edges match within the specified tolerance.
    for (size_t i = 0; i < referenceEdges.size(); ++i)
    {
        complex_t referenceImpedance = referenceEdges.at(i).z_c;
        complex_t distortedImpedance = distortedEdges.at(i).z_c;

        CHECK(std::abs(referenceImpedance.real() - distortedImpedance.real()) <= IMPEDANCE_SOLVE_TOLERANCE);
        CHECK(std::abs(referenceImpedance.imag() - distortedImpedance.imag()) <= IMPEDANCE_SOLVE_TOLERANCE);
    }
}


void validateNetworkImpedancesRegError(const ParameterRegTestConfig &config)
{
    auto refNetwork = loadNetwork(config.referenceNetworkPath);
    auto distNetwork = loadNetwork(config.distortedNetworkPath);

    std::ostringstream logStream;
    CppLogger logger(logStream);
    SolverSettings settings{};
    settings.ols_precision = config.threshold;
    settings.max_iterations_ols = config.maxIterations;

    PowerFlowSolver solver(distNetwork, settings, &logger);
    
    // Capture cerr
    std::stringstream cerrBuffer;
    std::streambuf* oldCerr = std::cerr.rdbuf(cerrBuffer.rdbuf());

    auto slackVoltages = config.slackVoltages;
    auto measuredValues = config.measuredValues;
    solver.solveParamsOLS(measuredValues, slackVoltages);

    // Restore cerr
    std::cerr.rdbuf(oldCerr);
    std::string solverOutput = cerrBuffer.str();

    if (config.error.type == ExpectationType::Contains)
    {
        CHECK(solverOutput.find(config.error.message) != std::string::npos);
    }
    else if (config.error.type == ExpectationType::DoesNotContain)
    {
        CHECK(solverOutput.find(config.error.message) == std::string::npos);
    }
}
