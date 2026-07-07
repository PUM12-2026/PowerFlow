add_library(PowerFlowLib STATIC  NetworkLoader.cpp "GridAnalyzer.cpp" "PowerFlowSolver.cpp" "computers/GradientComputer.cpp" "solvers/GaussSeidelSolver.cpp" "solvers/BackwardForwardSweepSolver.cpp" "solvers/ZBusJacobiSolver.cpp" "logger/Logger.cpp" "NetworkLoaderError.cpp" "NetworkValidator.cpp" "logger/CppLogger.cpp" "solvers/GridSolver.cpp" "ParameterValidator.cpp" "NetworkEditor.cpp")
if(CODE_COVERAGE)
    target_compile_options(PowerFlowLib PRIVATE -O0 -g --coverage -fprofile-arcs -ftest-coverage)
    target_link_options(PowerFlowLib PRIVATE --coverage)
endif()

# lib/Eigen
target_include_directories(PowerFlowLib PUBLIC ${PROJECT_SOURCE_DIR}/include PUBLIC ${PROJECT_SOURCE_DIR}/lib PUBLIC ${PROJECT_SOURCE_DIR}/lib/Eigen/unsupported)

# SCS
find_package(Threads REQUIRED)

find_package(scs)

if(scs_FOUND)
    message(STATUS "SCS found: LAD parameter estimation enabled.")
    target_compile_definitions(PowerFlowLib PRIVATE SCS_AVAILABLE)
    target_link_libraries(PowerFlowLib PRIVATE scs::scsdir)
else()
    message(WARNING "SCS not found: LAD parameter estimation disabled.")
endif()
