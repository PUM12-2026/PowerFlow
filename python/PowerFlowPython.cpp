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

    void reset()
    {
        solver->reset();
    }

private:
    std::unique_ptr<PowerFlowSolver> solver;
    CppLogger cpp_logger{};
};

PYBIND11_MODULE(PowerFlowPython, m)
{
    pybind11::class_<SolverSettings>(m, "SolverSettings")
        .def(pybind11::init<>())
        .def_readwrite("max_iterations_total", &SolverSettings::max_iterations_total)
        .def_readwrite("max_iterations_gauss", &SolverSettings::max_iterations_gauss)
        .def_readwrite("gauss_seidel_precision", &SolverSettings::gauss_seidel_precision)
        .def_readwrite("max_iterations_bfs", &SolverSettings::max_iterations_bfs)
        .def_readwrite("bfs_precision", &SolverSettings::bfs_precision)
        .def_readwrite("max_iterations_zbusjacobi", &SolverSettings::max_iterations_zbusjacobi)
        .def_readwrite("zbusjacobi_precision", &SolverSettings::zbusjacobi_precision);

    pybind11::class_<PowerFlow>(m, "PowerFlow")
        .def(pybind11::init<const std::string &, const SolverSettings &>(), pybind11::arg("filePath"), pybind11::arg_v("settings", SolverSettings(), "SolverSettings()"))
        .def("solve", &PowerFlow::solve, pybind11::arg("P"), pybind11::arg("V"), "Solve the power flow problem")
        .def("getLoadVoltages", &PowerFlow::getLoadVoltages, "Get the LOAD node voltages")
        .def("getAllVoltages", &PowerFlow::getAllVoltages, "Get all node voltages")
        .def("getCurrents", &PowerFlow::getCurrents, "Get currents")
        .def("getSlackPowers", &PowerFlow::getSlackPowers, "Get SLACK_IMPLICIT/SLACK powers")
        .def("reset", &PowerFlow::reset, "Reset network powers and voltages");
}
