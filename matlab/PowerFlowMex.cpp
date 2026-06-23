#include <unordered_map>
#include <fstream>
#include <memory>
#include <cstdint>

#include "powerflow/NetworkLoader.hpp"
#include "powerflow/PowerFlowSolver.hpp"
#include "powerflow/network.hpp"
#include "powerflow/logger/Logger.hpp"

#include "mexAdapter.hpp"
#include "mex.hpp"

// Logger that prints to the Matlab console.
class MatlabLogger : public Logger
{
public:
    MatlabLogger(std::shared_ptr<matlab::engine::MATLABEngine> matlab_ptr, LogLevel log_level)
        : Logger{log_level}, matlab_ptr{matlab_ptr} {}

private:
    std::shared_ptr<matlab::engine::MATLABEngine> matlab_ptr{};

    void flush() override
    {
        matlab::data::ArrayFactory factory;
        matlab_ptr->feval(u"fprintf", 0,
                          std::vector<matlab::data::Array>({factory.createScalar(ss.str())}));
        ss.str("");
        ss.clear();
    }
};

// PowerFlow Matlab MEX interface.
class MexFunction : public matlab::mex::Function
{
    std::unordered_map<std::uint64_t, std::unique_ptr<PowerFlowSolver>> solvers;

    /*
        Only increases the more handlers we have, and never decreases if a handler is no longer active.
    */
    std::uint64_t handleCounter = 0;

    // Pointer to MATLAB engine
    std::shared_ptr<matlab::engine::MATLABEngine> matlabPtr = getEngine();

    MatlabLogger logger{getEngine(), LogLevel::DEBUG};

public:
    /**
     * @brief Entry point for the MATLAB function call.
     * * @param outputs List of variables returned to the MATLAB workspace.
     * @param inputs  List of arguments passed from the MATLAB command line.
     */
    void operator()(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (inputs.size() < 1 || inputs[0].getType() != matlab::data::ArrayType::MATLAB_STRING)
        {
            throw std::invalid_argument("Missing first argument: command");
        }

        // Access the first element of the first input argument, since
        // MATLAB treats all inputs as arrays.
        std::string command = inputs[0][0];

        if (command == "load")
        {
            loadNetwork(outputs, inputs);
        }
        else if (command == "save")
        {
            save(outputs, inputs);
        }
        else if (command == "solve")
        {
            solve(outputs, inputs);
        }
        else if (command == "solveParams")
        {
            solveParams(outputs, inputs);
        }
        else if (command == "solveParamsOLS")
        {
            solveParamsOLS(outputs, inputs);
        }
        else if (command == "getLoadVoltages")
        {
            getLoadVoltages(outputs, inputs);
        }
        else if (command == "getAllVoltages")
        {
            getAllVoltages(outputs, inputs);
        }
        else if (command == "getCurrents")
        {
            getCurrents(outputs, inputs);
        }
        else if (command == "getSlackPowers")
        {
            getSlackPowers(outputs, inputs);
        }
        else if (command == "getImpedances")
        {
            getImpedances(outputs, inputs);
        }
        else if (command == "reset")
        {
            resetNetwork(outputs, inputs);
        }
        else if (command == "unload")
        {
            unloadNetwork(outputs, inputs);
        }
        else if (command == "getDvDs")
        {
            getDvDs(outputs, inputs);
        }
        else if (command == "getDiDs")
        {
            getDiDs(outputs, inputs);
        }
        else if (command == "getDsDs")
        {
            getDsDs(outputs, inputs);
        }
        else if (command == "getDslossDs")
        {
            getDslossDs(outputs, inputs);
        }
        else if (command == "isRadial")
        {
            isRadial(outputs, inputs);
        }
        else
        {
            throw std::invalid_argument("Invalid command");
        }
    }

private:
    /**
     * @brief Gets the solver handle ID from the input arguments.
     * * @param inputs  MATLAB input arguments containing the handle.
     * @return std::uint64_t The validated solver handle ID.
     */
    std::uint64_t getSolverHandle(matlab::mex::ArgumentList inputs)
    {
        if (inputs.size() < 2 ||
            inputs[1].getType() != matlab::data::ArrayType::UINT64 ||
            inputs[1].getNumberOfElements() != 1 ||
            solvers.count(inputs[1][0]) == 0)
        {
            throw std::invalid_argument("Invalid or missing Network handle");
        }
        return inputs[1][0];
    }

    void loadNetwork(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (inputs.size() < 2 || inputs[1].getType() != matlab::data::ArrayType::MATLAB_STRING)
        {
            throw std::invalid_argument("Missing file path");
        }

        std::string filePath = inputs[1][0];
        std::ifstream file(filePath);

        if (!file)
        {
            throw std::runtime_error("Could not open Network file");
        }

        SolverSettings settings{};

        // Load options struct.
        if (inputs.size() >= 3)
        {
            if (inputs[2].getType() != matlab::data::ArrayType::STRUCT || inputs[2].getNumberOfElements() != 1)
            {
                throw std::invalid_argument("Settings not a valid Matlab struct");
            }

            matlab::data::StructArray options = inputs[2];

            for (std::string fieldName : options.getFieldNames())
            {
                const matlab::data::Array field = options[0][fieldName];

                if (fieldName == "max_iterations_total")
                {
                    if (field.getType() != matlab::data::ArrayType::DOUBLE || field.getNumberOfElements() != 1)
                    {
                        throw std::invalid_argument("Invalid max_iterations_total");
                    }
                    settings.max_iterations_total = field[0];
                }
                else if (fieldName == "max_iterations_gauss")
                {
                    if (field.getType() != matlab::data::ArrayType::DOUBLE || field.getNumberOfElements() != 1)
                    {
                        throw std::invalid_argument("Invalid max_iterations_gauss");
                    }
                    settings.max_iterations_gauss = field[0];
                }
                else if (fieldName == "gauss_seidel_precision")
                {
                    if (field.getType() != matlab::data::ArrayType::DOUBLE || field.getNumberOfElements() != 1)
                    {
                        throw std::invalid_argument("Invalid gauss_seidel_precision");
                    }
                    settings.gauss_seidel_precision = field[0];
                }
                else if (fieldName == "max_iterations_bfs")
                {
                    if (field.getType() != matlab::data::ArrayType::DOUBLE || field.getNumberOfElements() != 1)
                    {
                        throw std::invalid_argument("Invalid max_iterations_bfs");
                    }
                    settings.max_iterations_bfs = field[0];
                }
                else if (fieldName == "bfs_precision")
                {
                    if (field.getType() != matlab::data::ArrayType::DOUBLE || field.getNumberOfElements() != 1)
                    {
                        throw std::invalid_argument("Invalid bfs_precision");
                    }
                    settings.bfs_precision = field[0];
                }
                else if (fieldName == "compute_gradients")
                {
                    if (field.getType() != matlab::data::ArrayType::LOGICAL || field.getNumberOfElements() != 1)
                    {
                        throw std::invalid_argument("Invalid compute_gradients");
                    }
                    settings.compute_gradients = field[0];
                }
                else if (fieldName == "max_iterations_zbusjacobi")
                {
                    if (field.getType() != matlab::data::ArrayType::DOUBLE || field.getNumberOfElements() != 1)
                    {
                        throw std::invalid_argument("Invalid max_iterations_zbusjacobi");
                    }
                    settings.max_iterations_zbusjacobi = field[0];
                }
                else if (fieldName == "zbusjacobi_precision")
                {
                    if (field.getType() != matlab::data::ArrayType::DOUBLE || field.getNumberOfElements() != 1)
                    {
                        throw std::invalid_argument("Invalid zbusjacobi_precision");
                    }
                    settings.zbusjacobi_precision = field[0];
                }
                else if (fieldName == "max_iterations_ols")
                {
                    if (field.getType() != matlab::data::ArrayType::DOUBLE || field.getNumberOfElements() != 1)
                    {
                        throw std::invalid_argument("Invalid max_iterations_ols");
                    }
                    settings.max_iterations_ols = field[0];
                }
                else if (fieldName == "ols_precision")
                {
                    if (field.getType() != matlab::data::ArrayType::DOUBLE || field.getNumberOfElements() != 1)
                    {
                        throw std::invalid_argument("Invalid ols_precision");
                    }
                    settings.ols_precision = field[0];
                }
                else
                {
                    throw std::invalid_argument("Invalid option " + fieldName + " in setting struct");
                }
            }
        }

        NetworkLoader loader(file);
        std::shared_ptr<Network> net = loader.loadNetwork();

        // Ensure our solver handle can be referenced to later via an handle ID from MATLAB, since MATLAB iself
        // cannot fully keep track of this.
        solvers.insert({handleCounter, std::make_unique<PowerFlowSolver>(net, settings, &logger)});
        std::uint64_t handle = handleCounter;
        ++handleCounter;

        // Send the solver handle up to MATLAB.
        matlab::data::ArrayFactory factory;
        outputs[0] = factory.createScalar<std::uint64_t>(handle);
    }

    void save(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (inputs.size() < 3 || inputs[2].getType() != matlab::data::ArrayType::MATLAB_STRING)
        {
            throw std::invalid_argument("Missing file path");
        }

        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        std::string filePath = inputs[2][0];
        std::ofstream file(filePath);

        solver->save(file);
    }

    void solve(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        if (inputs.size() < 3 || inputs[2].getType() != matlab::data::ArrayType::COMPLEX_DOUBLE)
        {
            throw std::invalid_argument("Missing or invalid S vector");
        }
        if (inputs.size() < 4 || (inputs[3].getType() != matlab::data::ArrayType::COMPLEX_DOUBLE && !inputs[3].isEmpty()))
        {
            throw std::invalid_argument("Missing or invalid V vector");
        }

        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        matlab::data::TypedArray<complex_t> matlabS = inputs[2];
        matlab::data::TypedArray<complex_t> matlabV = inputs[3];

        std::vector<complex_t> S(matlabS.begin(), matlabS.end());
        std::vector<complex_t> V(matlabV.begin(), matlabV.end());

        solver->solve(S, V);
    }

    void solveParamsOLS(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        matlab::data::ArrayFactory factory;
        matlab::data::TypedArray<node_idx_t> keys = inputs[2];
        matlab::data::TypedArray<complex_t> voltages = inputs[3];
        matlab::data::TypedArray<complex_t> powerInjections = inputs[4];
        matlab::data::TypedArray<complex_t> slackVoltages_ = inputs[5];

        size_t samples = voltages.getDimensions()[1];

        std::unordered_map<node_idx_t, MeasuredValues> measuredValues;
        for (size_t i = 0; i < keys.getNumberOfElements(); i++)
        {
            MeasuredValues val;

            for (size_t j = 0; j < samples; j++)
            {
                val.U.push_back(voltages[i][j]);
                val.S.push_back(powerInjections[i][j]);
            }

            measuredValues[keys[i]] = std::move(val);
        }

        std::vector<complex_t> slackVoltages(slackVoltages_.begin(), slackVoltages_.end());
        std::vector<complex_t> Z = solver->solveParamsOLS(measuredValues, slackVoltages);

        outputs[0] = factory.createArray({1, Z.size()}, Z.begin(), Z.end());
    }

    void solveParams(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        matlab::data::ArrayFactory factory;
        matlab::data::TypedArray<node_idx_t> keys = inputs[2];
        matlab::data::TypedArray<complex_t> vals = inputs[3];
        double precision = inputs[4][0];

        std::unordered_map<node_idx_t, complex_t> V;
        for (size_t i = 0; i < keys.getNumberOfElements(); i++)
        {
            V[keys[i]] = vals[i];
        }
        solver->solveParams(V, precision);
    }

    void getLoadVoltages(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        std::vector<complex_t> V = solver->getLoadVoltages();
        matlab::data::ArrayFactory factory;
        outputs[0] = factory.createArray({1, V.size()}, V.begin(), V.end());
    }

    void getAllVoltages(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        std::vector<complex_t> V = solver->getAllVoltages();
        matlab::data::ArrayFactory factory;
        outputs[0] = factory.createArray({1, V.size()}, V.begin(), V.end());
    }

    void getCurrents(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        std::vector<complex_t> I = solver->getCurrents();
        matlab::data::ArrayFactory factory;
        outputs[0] = factory.createArray({1, I.size()}, I.begin(), I.end());
    }

    void getSlackPowers(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        std::vector<complex_t> S = solver->getSlackPowers();
        matlab::data::ArrayFactory factory;
        outputs[0] = factory.createArray({1, S.size()}, S.begin(), S.end());
    }

    void getImpedances(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        std::vector<complex_t> Z = solver->getImpedances();
        matlab::data::ArrayFactory factory;
        outputs[0] = factory.createArray({1, Z.size()}, Z.begin(), Z.end());
    }

    void getDvDs(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        std::vector<std::vector<std::array<double, 2>>> dVdS = solver->getDvDs();
        size_t rows = dVdS.size();
        size_t cols = rows > 0 ? dVdS[0].size() : 0;
        matlab::data::ArrayFactory factory;
        matlab::data::TypedArray<double> out = factory.createArray<double>({rows, cols, 2});
        for (size_t i = 0; i < rows; ++i)
        {
            for (size_t j = 0; j < cols; ++j)
            {
                out[i][j][0] = dVdS[i][j][0]; // Real value
                out[i][j][1] = dVdS[i][j][1]; // Imaginary value
            }
        }

        outputs[0] = std::move(out);
    }

    void getDiDs(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        std::vector<std::vector<std::array<double, 2>>> dIdS = solver->getDiDs();
        size_t rows = dIdS.size();
        size_t cols = rows > 0 ? dIdS[0].size() : 0;
        matlab::data::ArrayFactory factory;
        matlab::data::TypedArray<double> out = factory.createArray<double>({rows, cols, 2});
        for (size_t i = 0; i < rows; ++i)
        {
            for (size_t j = 0; j < cols; ++j)
            {
                out[i][j][0] = dIdS[i][j][0]; // Real value
                out[i][j][1] = dIdS[i][j][1]; // Imaginary value
            }
        }

        outputs[0] = std::move(out);
    }

    void getDsDs(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        std::vector<std::vector<std::array<double, 2>>> dSdS = solver->getDsDs();
        size_t rows = dSdS.size();
        size_t cols = rows > 0 ? dSdS[0].size() : 0;
        matlab::data::ArrayFactory factory;
        matlab::data::TypedArray<double> out = factory.createArray<double>({rows, cols, 2});
        for (size_t i = 0; i < rows; ++i)
        {
            for (size_t j = 0; j < cols; ++j)
            {
                out[i][j][0] = dSdS[i][j][0]; // Real value
                out[i][j][1] = dSdS[i][j][1]; // Imaginary value
            }
        }

        outputs[0] = std::move(out);
    }

    void getDslossDs(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        std::vector<std::vector<std::array<double, 2>>> dSlossdS = solver->getDslossDs();
        size_t rows = dSlossdS.size();
        size_t cols = rows > 0 ? dSlossdS[0].size() : 0;
        matlab::data::ArrayFactory factory;
        matlab::data::TypedArray<double> out = factory.createArray<double>({rows, cols, 2});
        for (size_t i = 0; i < rows; ++i)
        {
            for (size_t j = 0; j < cols; ++j)
            {
                out[i][j][0] = dSlossdS[i][j][0]; // Real value
                out[i][j][1] = dSlossdS[i][j][1]; // Imaginary value
            }
        }

        outputs[0] = std::move(out);
    }

    void isRadial(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        matlab::data::ArrayFactory factory;
        outputs[0] = factory.createScalar<bool>(solver->isRadial());
    }

    void resetNetwork(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::unique_ptr<PowerFlowSolver> &solver = solvers.at(getSolverHandle(inputs));
        solver->reset();
    }

    void unloadNetwork(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs)
    {
        std::uint64_t handle = getSolverHandle(inputs);

        if (solvers.count(handle) == 0)
        {
            throw std::runtime_error("Network not loaded");
        }
        else
        {
            // Make sure we no longer track this solver handle.
            solvers.erase(handle);
        }
    }

    void printToMatlab(const std::ostringstream &message)
    {
        matlab::data::ArrayFactory factory;
        matlabPtr->feval(u"fprintf", 0, std::vector<matlab::data::Array>({factory.createScalar(message.str())}));
    }
};
