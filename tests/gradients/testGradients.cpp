#include "../tests/catch.hpp"
#include "../utils.hpp"
#include "gradientUtils.hpp"

TEST_CASE("U-01.1: Validate that BFS solver computes analytical gradients during solve.", "[unit][gradient]")
{
    GradientTestEnvironment environment = setupEnvironment("examples/example_network_single_grid.txt");

    // This example only has one grid, so we only run the test on that grid.
    Grid &grid = environment.network->grids.at(0);

    // We have three load nodes in this example.
    std::vector<complex_t> S = {{0.005, 0.004}, {0.004, 0.002}, {0.002, 0.001}};

    // We have one voltage source, i.e. the SLACK node.
    std::vector<complex_t> V = {{1, 0}};

    // Ensure gradient vectors are empty or uninitialized before solving.
    REQUIRE(grid.dVdS.empty());
    REQUIRE(grid.dSdS.empty());
    REQUIRE(grid.dIdS.empty());
    REQUIRE(grid.dSlossdS.empty());

    environment.pfs->solve(S, V);

    // Verify that gradients are calculated and populated in the grid object.
    REQUIRE_FALSE(grid.dVdS.empty());
    REQUIRE_FALSE(grid.dSdS.empty());
    REQUIRE_FALSE(grid.dIdS.empty());
    REQUIRE_FALSE(grid.dSlossdS.empty());
}

TEST_CASE("U-01.2: Validate that resetting the BFS solver clears all analytical gradient matrices.", "[unit][gradient]")
{
    GradientTestEnvironment environment = setupEnvironment("examples/example_network_single_grid.txt");

    // This example only has one grid, so we only run the test on that grid.
    Grid &grid = environment.network->grids.at(0);

    // We have three load nodes in this example.
    std::vector<complex_t> S = {{0.005, 0.004}, {0.004, 0.002}, {0.002, 0.001}};

    // We have one voltage source, i.e. the SLACK node.
    std::vector<complex_t> V = {{1, 0}};

    // Ensure gradient vectors are empty or uninitialized before solving.
    REQUIRE(grid.dVdS.empty());
    REQUIRE(grid.dSdS.empty());
    REQUIRE(grid.dIdS.empty());
    REQUIRE(grid.dSlossdS.empty());

    environment.pfs->solve(S, V);

    // Verify that gradients are calculated and populated in the grid object.
    REQUIRE_FALSE(grid.dVdS.empty());
    REQUIRE_FALSE(grid.dSdS.empty());
    REQUIRE_FALSE(grid.dIdS.empty());
    REQUIRE_FALSE(grid.dSlossdS.empty());

    environment.pfs->reset();

    // Verify that after resetting the solver, the gradients are cleared.
    REQUIRE(grid.dVdS.empty());
    REQUIRE(grid.dSdS.empty());
    REQUIRE(grid.dIdS.empty());
    REQUIRE(grid.dSlossdS.empty());
}

TEST_CASE("U-01.3: Validate that BFS gradient matrix dimensions match network topology and leaf node count.", "[unit][gradient]")
{
    GradientTestEnvironment environment = setupEnvironment("examples/example_network_single_grid.txt");

    // This example only has one grid, so we only run the test on that grid.
    Grid &grid = environment.network->grids.at(0);

    // We have three load nodes in this example.
    std::vector<complex_t> S = {{0.005, 0.004}, {0.004, 0.002}, {0.002, 0.001}};

    // We have one voltage source, i.e. the SLACK node.
    std::vector<complex_t> V = {{1, 0}};

    environment.pfs->solve(S, V);

    std::vector<std::pair<size_t, node_idx_t>> networkLeafNodes = getNetworkLeafNodeIndices(*environment.network);

    // Verify that each gradient vector has the correct size corresponding to the number of leaf nodes in the network.
    for (const auto &dVdSNode : environment.pfs->getDvDs())
    {
        REQUIRE(dVdSNode.size() == networkLeafNodes.size());
    }

    for (const auto &dSdSNode : environment.pfs->getDsDs())
    {
        REQUIRE(dSdSNode.size() == networkLeafNodes.size());
    }

    for (const auto &dIdSNode : environment.pfs->getDiDs())
    {
        REQUIRE(dIdSNode.size() == networkLeafNodes.size());
    }

    for (const auto &dSlossdSNode : environment.pfs->getDslossDs())
    {
        REQUIRE(dSlossdSNode.size() == networkLeafNodes.size());
    }

    // Verify that dVdS have gradients for all nodes except the root node, which has no dV/dS sensitivities.
    REQUIRE(environment.pfs->getDvDs().size() == grid.nodes.size() - 1);

    // Verify that dSdS have gradients for all nodes except the load nodes, which have no dS/dS sensitivities.
    REQUIRE(environment.pfs->getDsDs().size() == (grid.nodes.size() - networkLeafNodes.size()));

    // Verify that dIdS and dSlossDs have gradients for all edges in the grid.
    REQUIRE(environment.pfs->getDiDs().size() == grid.edges.size());
    REQUIRE(environment.pfs->getDslossDs().size() == grid.edges.size());
}

TEST_CASE("U-01.4: Validate that BFS getters throw error when compute_gradients flag is disabled.", "[unit][gradient]")
{
    GradientTestEnvironment environment = setupEnvironment("examples/example_network_single_grid.txt", nullptr, false);

    // We have three load nodes in this example.
    std::vector<complex_t> S = {{0.005, 0.004}, {0.004, 0.002}, {0.002, 0.001}};

    // We have one voltage source, i.e. the SLACK node.
    std::vector<complex_t> V = {{1, 0}};

    environment.pfs->solve(S, V);

    const std::string expectedMsg = "Gradients are unavailable. Enable 'compute_gradients' in settings and ensure the network uses Backward Forward Sweep.";

    // Verify that all gradient getters throw the expected runtime error message when compute_gradients is false.
    REQUIRE_THROWS_WITH(environment.pfs->getDvDs(), expectedMsg);
    REQUIRE_THROWS_WITH(environment.pfs->getDsDs(), expectedMsg);
    REQUIRE_THROWS_WITH(environment.pfs->getDiDs(), expectedMsg);
    REQUIRE_THROWS_WITH(environment.pfs->getDslossDs(), expectedMsg);
}

TEST_CASE("U-01.5: Validate that BFS getters throw error when non-BFS solvers are utilized.", "[unit][gradient]")
{
    std::stringstream logOutput;
    CppLogger logger(logOutput);

    // Networks with cycles will trigger the Gauss-Seidel solver, which does not support analytical gradients.
    GradientTestEnvironment environment = setupEnvironment("examples/test_networks/test_network_cycle.txt", &logger);

    // We have three load nodes in this example.
    std::vector<complex_t> S = {{0.005, 0.004}, {0.004, 0.002}, {0.002, 0.001}};

    // We have one voltage source, i.e. the SLACK node.
    std::vector<complex_t> V = {{1, 0}};

    environment.pfs->solve(S, V);

    // Check that the log contains the expected message.
    REQUIRE(logOutput.str().find("Gradients will not be computed.") != std::string::npos);

    const std::string expectedMsg = "Gradients are unavailable. Enable 'compute_gradients' in settings and ensure the network uses Backward Forward Sweep.";

    // Verify that all gradient getters throw the expected runtime error message when a non-BFS solver is used.
    REQUIRE_THROWS_WITH(environment.pfs->getDvDs(), expectedMsg);
    REQUIRE_THROWS_WITH(environment.pfs->getDsDs(), expectedMsg);
    REQUIRE_THROWS_WITH(environment.pfs->getDiDs(), expectedMsg);
    REQUIRE_THROWS_WITH(environment.pfs->getDslossDs(), expectedMsg);
}

TEST_CASE("F-01: Validate that BFS analytical gradients match numerical sensitivities on single-grid tree networks", "[functional][gradient]")
{
    GradientTestEnvironment environment = setupEnvironment("examples/example_network_single_grid.txt");

    // We have three load nodes in this example.
    std::vector<complex_t> S = {{0.005, 0.004}, {0.004, 0.002}, {0.002, 0.001}};

    // We have one voltage source, i.e. the SLACK node.
    std::vector<complex_t> V = {{1, 0}};

    environment.pfs->solve(S, V);

    runGradientTests(environment, 0, S, V);
}

TEST_CASE("F-02: Validate that BFS analytical gradients match numerical sensitivities on multi-subgrid tree networks.", "[functional][gradient]")
{
    GradientTestEnvironment environment = setupEnvironment("examples/example_network.txt");

    // We have three load nodes in this example.
    std::vector<complex_t> S = {{0.005, 0.004}, {0.004, 0.002}, {0.002, 0.001}};

    // We have one voltage source, i.e. the SLACK node.
    std::vector<complex_t> V = {{1, 0}};

    environment.pfs->solve(S, V);

    for (size_t gridIdx = 0; gridIdx < environment.network->grids.size(); ++gridIdx)
    {
        runGradientTests(environment, gridIdx, S, V);
    }
}