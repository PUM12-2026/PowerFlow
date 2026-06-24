#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>

#include <memory>
#include <complex>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>

#include "powerflow/network.hpp"
#include "powerflow/NetworkLoader.hpp"
#include "powerflow/PowerFlowSolver.hpp"
#include "powerflow/logger/CppLogger.hpp"

// PowerFlow Python interface class.
class PowerFlow
{
public:
    PowerFlow(const std::string &filePath, const SolverSettings &settings)
    {
        std::ifstream file(filePath);

        if (!file)
        {
            throw std::runtime_error("Could not open Network file: " + filePath);
        }

        NetworkLoader loader(file);
        std::unique_ptr<Network> network = loader.loadNetwork();
        solver = std::make_unique<PowerFlowSolver>(std::move(network), settings, &cpp_logger);
    }

    void solve(std::vector<std::complex<double>> &S, std::vector<std::complex<double>> &V)
    {
        solver->solve(S, V);
    }

    void solveParams(std::unordered_map<node_idx_t, complex_t> &V, double precision)
    {
        solver->solveParams(V, precision);
    }

    std::vector<complex_t> solveParamsOLS(std::unordered_map<node_idx_t, std::vector<complex_t>> &measuredVoltages, 
        std::unordered_map<node_idx_t, std::vector<complex_t>> &measuredPowerInjections,
        std::vector<complex_t> &slackVoltages)
    {
        std::unordered_map<node_idx_t, MeasuredValues> measuredValues;
        for (auto &[key, U] : measuredVoltages)
        {
            measuredValues[key] = MeasuredValues{U, measuredPowerInjections.at(key)};
        }
        return solver->solveParamsOLS(measuredValues, slackVoltages);
    }

    std::vector<complex_t> getLoadVoltages() const
    {
        return solver->getLoadVoltages();
    }

    std::vector<complex_t> getAllVoltages() const
    {
        return solver->getAllVoltages();
    }

    std::vector<complex_t> getCurrents() const
    {
        return solver->getCurrents();
    }

    std::vector<complex_t> getSlackPowers() const
    {
        return solver->getSlackPowers();
    }

    std::vector<complex_t> getImpedances() const
    {
        return solver->getImpedances();
    }

    void setImpedances(std::vector<complex_t> Z)
    {
        solver->setImpedances(Z);
    }

    std::vector<std::vector<std::array<double, 2>>> getDvDs() const
    {
        return solver->getDvDs();
    }

    std::vector<std::vector<std::array<double, 2>>> getDiDs() const
    {
        return solver->getDiDs();
    }

    std::vector<std::vector<std::array<double, 2>>> getDsDs() const
    {
        return solver->getDsDs();
    }

    std::vector<std::vector<std::array<double, 2>>> getDslossDs() const
    {
        return solver->getDslossDs();
    }

    void reset()
    {
        solver->reset();
    }

    void save(std::string filepath)
    {
        std::ofstream file(filepath);
        solver->save(file);
    }

    bool isRadial()
    {
        return solver->isRadial();
    }

    void simplifyNetwork()
    {
        solver->simplifyNetwork();
    }

private:
    std::unique_ptr<PowerFlowSolver> solver;
    CppLogger cpp_logger{};
};

PYBIND11_MODULE(PowerFlowPython, m)
{
    m.doc() = "A library for pwer flow computations. It can: \n\
    - Estimate state in radial and non-radial networks. \n\
    - Compute gradients in radial networks analytically. \n\
    - Estimate cable parameters in radial networks.";

    pybind11::class_<SolverSettings>(m, "SolverSettings")
        .def(pybind11::init<>())
        .def_readwrite("max_iterations_total", &SolverSettings::max_iterations_total)
        .def_readwrite("max_iterations_gauss", &SolverSettings::max_iterations_gauss)
        .def_readwrite("gauss_seidel_precision", &SolverSettings::gauss_seidel_precision)
        .def_readwrite("max_iterations_bfs", &SolverSettings::max_iterations_bfs)
        .def_readwrite("bfs_precision", &SolverSettings::bfs_precision)
        .def_readwrite("compute_gradients", &SolverSettings::compute_gradients)
        .def_readwrite("max_iterations_zbusjacobi", &SolverSettings::max_iterations_zbusjacobi)
        .def_readwrite("zbusjacobi_precision", &SolverSettings::zbusjacobi_precision)
        .def_readwrite("max_iterations_ols", &SolverSettings::max_iterations_ols)
        .def_readwrite("ols_precision", &SolverSettings::ols_precision);

    pybind11::class_<PowerFlow>(m, "PowerFlow")
        .def(pybind11::init<const std::string &, const SolverSettings &>(), pybind11::arg("filePath"), pybind11::arg_v("settings", SolverSettings(), "SolverSettings()"))
        .def("solve", &PowerFlow::solve, pybind11::arg("P"), pybind11::arg("V"), "Solve the power flow problem")
        .def("solveParams", &PowerFlow::solveParams, pybind11::arg("V"), pybind11::arg("precision"), "Find and adjust invalid cable parameters. WARNING: Not recommended, use solveParamsOLS instead.")
        .def("solveParamsOLS", &PowerFlow::solveParamsOLS, pybind11::arg("measuredVoltages"), pybind11::arg("measuredPowerInjections"), pybind11::arg("slackVoltages"), "Estimate cable parameters in grid using regression.")
        .def("getLoadVoltages", &PowerFlow::getLoadVoltages, "Get the LOAD node voltages")
        .def("getAllVoltages", &PowerFlow::getAllVoltages, "Get all node voltages")
        .def("getCurrents", &PowerFlow::getCurrents, "Get currents")
        .def("getSlackPowers", &PowerFlow::getSlackPowers, "Get SLACK_IMPLICIT/SLACK powers")
        .def("getImpedances", &PowerFlow::getImpedances, "Get impedances")
        .def("setImpedances", &PowerFlow::setImpedances, "Set impedances")
        .def("getDvDs", &PowerFlow::getDvDs, "Get voltage gradients of all nodes except root node w.r.t. power of all LOAD nodes")
        .def("getDiDs", &PowerFlow::getDiDs, "Get current gradients of all edges w.r.t. power of all LOAD nodes")
        .def("getDsDs", &PowerFlow::getDsDs, "Get power gradients of all nodes except LOAD nodes w.r.t. power all of LOAD nodes")
        .def("getDslossDs", &PowerFlow::getDslossDs, "Get power loss gradients of all edges w.r.t. power all of LOAD nodes")
        .def("reset", &PowerFlow::reset, "Reset network powers and voltages")
        .def("save", &PowerFlow::save, "Saves the network to file")
        .def("isRadial", &PowerFlow::isRadial, "Checks whether the network is radial. Returns true if it is, else returns false")
        .def("simplifyNetwork", &PowerFlow::simplifyNetwork, "Simplifies the network by removing pass-through nodes. Network must be radial.");
}
