#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include "powerflow/NetworkLoader.hpp"
#include "powerflow/solvers/BackwardForwardSweepSolver.hpp"
#include "powerflow/solvers/GaussSeidelSolver.hpp"
#include "powerflow/solvers/ZBusJacobiSolver.hpp"
#include "powerflow/PowerFlowSolver.hpp"
#include "powerflow/GridAnalyzer.hpp"
#include "powerflow/logger/CppLogger.hpp"

#include <fstream>
#include <string>
#include <filesystem>

std::string localPath = "";

// CHECK_FALSE(file.fail());  checks that the file can be opened correctly (in most cases it is the wrong filepath)
bool test_input_error_message(std::string errorMessage, std::string filePath)
{
    std::ifstream file(filePath);
    CHECK_FALSE(file.fail());
    NetworkLoader loader(file);
    REQUIRE_THROWS_WITH(loader.loadNetwork(), Catch::Matchers::Contains(errorMessage)); // should fail and halt so true is not returned
    return true;
}

TEST_CASE("Networkloader input", "[!throws]")
{
    std::cout << "LOCALPATH: " << localPath << std::endl;
    REQUIRE(test_input_error_message("Invalid S base", localPath + "examples/test_networks/invalid_base_S.txt"));
    REQUIRE(test_input_error_message("Invalid V base", localPath + "examples/test_networks/invalid_base_V.txt"));
    REQUIRE(test_input_error_message("Invalid command", localPath + "examples/test_networks/invalid_command.txt"));
    REQUIRE(test_input_error_message("Invalid node index", localPath + "examples/test_networks/invalid_node_index.txt"));
    REQUIRE(test_input_error_message("Empty grid", localPath + "examples/test_networks/empty_grid.txt"));
    REQUIRE(test_input_error_message("Invalid edge parent index", localPath + "examples/test_networks/invalid_edge_parent_index.txt"));
    REQUIRE(test_input_error_message("Invalid edge child index", localPath + "examples/test_networks/invalid_edge_child_index.txt"));
    REQUIRE(test_input_error_message("Invalid edge impedance", localPath + "examples/test_networks/invalid_edge_impedance_index.txt"));
    REQUIRE(test_input_error_message("Invalid grid index", localPath + "examples/test_networks/invalid_slack_grid_index.txt"));
    REQUIRE(test_input_error_message("Invalid LOAD_IMPLICIT node index", localPath + "examples/test_networks/invalid_slack_node_index.txt"));
    REQUIRE(test_input_error_message("Invalid grid index", localPath + "examples/test_networks/invalid_PQ_grid_index.txt"));
    REQUIRE(test_input_error_message("Invalid SLACK_IMPLICIT node index", localPath + "examples/test_networks/invalid_PQ_node_index.txt"));
    REQUIRE(test_input_error_message("Invalid node type", localPath + "examples/test_networks/invalid_node_type.txt"));
}

void validatePFSThrow(const std::string &filePath)
{
    std::ifstream file(filePath);
    CHECK_FALSE(file.fail());

    NetworkLoader loader(file);
    std::unique_ptr<Network> net = loader.loadNetwork();
    CppLogger logger(std::cout);
    SolverSettings settings{};
    PowerFlowSolver pfs(std::move(net), settings, &logger);
}

TEST_CASE("Network validation", "[!throws]")
{
    REQUIRE_THROWS_WITH(validatePFSThrow(localPath + "examples/test_networks/invalid_node_type_in_connection.txt"),
                        Catch::Matchers::Contains("Invalid node type in connection 0"));

    REQUIRE_THROWS_WITH(validatePFSThrow(localPath + "examples/test_networks/invalid_node_type_in_connection_2.txt"),
                        Catch::Matchers::Contains("Invalid node type in connection 1"));

    REQUIRE_THROWS_WITH(validatePFSThrow(localPath + "examples/test_networks/missing_connections.txt"),
                        Catch::Matchers::Contains("Grid 0 not properly connected to the rest of the network"));

    REQUIRE_THROWS_WITH(validatePFSThrow(localPath + "examples/test_networks/double_edge.txt"),
                        Catch::Matchers::Contains("More than one edge detected between node 0 and node 2 in grid 2"));

    REQUIRE_THROWS_WITH(validatePFSThrow(localPath + "examples/test_networks/double_connection.txt"),
                        Catch::Matchers::Contains("Grid 0 not properly connected to the rest of the network"));

    REQUIRE_THROWS_WITH(validatePFSThrow(localPath + "examples/test_networks/missing_slack.txt"),
                        Catch::Matchers::Contains("Missing slack node in grid 3"));

    REQUIRE_THROWS_WITH(validatePFSThrow(localPath + "examples/test_networks/inner_connection.txt"),
                        Catch::Matchers::Contains("Connection in the same grid 2 not allowed"));

    REQUIRE_THROWS_WITH(validatePFSThrow(localPath + "examples/test_networks/edge_same_node.txt"),
                        Catch::Matchers::Contains("Invalid edge 2 that connects to the same node in grid 1"));

    REQUIRE_THROWS_WITH(validatePFSThrow(localPath + "examples/test_networks/zero_impedance.txt"),
                        Catch::Matchers::Contains("Invalid zero impedance in edge 1 in grid 1"));

    // See how PowerFlow handles the situation of a network with one SLACK node
    // and a LOAD node that is not connected, i.e. a disjoint grid.
    REQUIRE_THROWS_WITH([]() {
        std::unique_ptr<Network> net = std::make_unique<Network>();
        Grid grid;
        GridNode node1;
        GridNode node2;
        node1.type = SLACK;
        node2.type = LOAD;
        grid.nodes.push_back(node1);
        grid.nodes.push_back(node2);
        net->grids.push_back(grid);
        CppLogger logger(std::cout);
        SolverSettings settings{};
        PowerFlowSolver pfs(std::move(net), settings, &logger); 
    }(), Catch::Matchers::Contains("Grid 0 consists of multiple disjoint graphs"));
}

TEST_CASE("Compare output of Backward-Forward Sweep and GaussSeidel", "[validation]")
{
    SECTION("test_network.txt")
    {
        // ----- Common Setup -----
        CppLogger logger(std::cout);

        double precision = 1e-10;
        int maxIterations = 10000;
        // ------------------------

        // ----- Backward-Forward Sweep (BFS) Setup -----
        // Load our test file and make sure it exists.
        std::ifstream bfsTestFile(localPath + "examples/test_networks/test_network.txt");
        CHECK_FALSE(bfsTestFile.fail());

        // Create a loader that loads in the network from the file.
        NetworkLoader bfsLoader(bfsTestFile);
        std::unique_ptr<Network> bfsNetwork = bfsLoader.loadNetwork();

        // Add all our BFS solvers for each subnetwork.
        std::vector<GridSolver*> bfsSolvers;
        for (Grid &grid : bfsNetwork->grids)
        {
            bfsSolvers.push_back(new BackwardForwardSweepSolver(&grid, &logger, maxIterations, precision));
        }

        // Load our power loads.
        bfsNetwork->grids.at(1).nodes.at(2).s = -complex_t(0.004, 0.002);
        bfsNetwork->grids.at(2).nodes.at(1).s = -complex_t(0.002, 0.001);
        bfsNetwork->grids.at(2).nodes.at(2).s = -complex_t(0.005, 0.004);

        // Run all of our BFS solvers.
        for (GridSolver *solver : bfsSolvers)
        {
            solver->solve();
        }
        // -----------------------------------------------

        // ----- Setup GaussSeidel (GS) -----
        // Load our test file and make sure it exists.
        std::ifstream gsTestFile(localPath + "examples/test_networks/test_network.txt");
        CHECK_FALSE(gsTestFile.fail());

        // Create a loader that loads in the network from the file.
        NetworkLoader gsLoader(gsTestFile);
        std::unique_ptr<Network> gsNetwork = gsLoader.loadNetwork();

        // Add all our GS solvers for each subnetwork.
        std::vector<GridSolver*> gsSolvers;
        for (Grid &grid : gsNetwork->grids)
        {
            gsSolvers.push_back(new GaussSeidelSolver(&grid, &logger, maxIterations, precision));
        }

        // Load our power loads.
        gsNetwork->grids.at(1).nodes.at(2).s = -complex_t(0.004, 0.002);
        gsNetwork->grids.at(2).nodes.at(1).s = -complex_t(0.002, 0.001);
        gsNetwork->grids.at(2).nodes.at(2).s = -complex_t(0.005, 0.004);

        // Run all of our GS solvers.
        for (GridSolver *solver : gsSolvers)
        {
            solver->solve();
        }
        // -----------------------------------------------

        // Compare BFS and GS networks solved against each other.
        for (unsigned long i = 0; i < gsNetwork->grids.size(); i++)
        {
            for (unsigned long j = 0; j < gsNetwork->grids[i].nodes.size(); j++)
            {
                if (gsNetwork->grids[i].nodes[j].type == NodeType::MIDDLE)
                {
                    continue;
                }

                // Check that BFS and GS lay within the same precision of each other using Catch2 Matcher.
                CHECK_THAT(gsNetwork->grids[i].nodes[j].v.real(), Catch::Matchers::WithinAbs(bfsNetwork->grids[i].nodes[j].v.real(), 1e-10));
                CHECK_THAT(gsNetwork->grids[i].nodes[j].v.imag(), Catch::Matchers::WithinAbs(bfsNetwork->grids[i].nodes[j].v.imag(), 1e-10));
                CHECK_THAT(gsNetwork->grids[i].nodes[j].s.real(), Catch::Matchers::WithinAbs(bfsNetwork->grids[i].nodes[j].s.real(), 1e-10));
                CHECK_THAT(gsNetwork->grids[i].nodes[j].s.imag(), Catch::Matchers::WithinAbs(bfsNetwork->grids[i].nodes[j].s.imag(), 1e-10));
            }
        }
    }
}

TEST_CASE("Compare GaussSeidel and ZBus Jacobi", "[validation]")
{
    // ----- Common Setup -----
    CppLogger logger(std::cout);

    int maxIterations = 100000;
    double precision = 1e-10;
    // ------------------------

    // ----- Setup GaussSeidel (GS) -----
    // Load our test file and make sure it exists.
    std::ifstream gsTestFile(localPath + "examples/test_networks/test_network_cycle.txt");
    CHECK_FALSE(gsTestFile.fail());

    // Create a loader that loads in the network from the file.
    NetworkLoader gsLoader(gsTestFile);
    std::unique_ptr<Network> gsNetwork = gsLoader.loadNetwork();

    // Add all our GS solvers for each subnetwork.
    std::vector<GridSolver*> gsSolvers;
    for (Grid &grid : gsNetwork->grids)
    {
        gsSolvers.push_back(new GaussSeidelSolver(&grid, &logger, maxIterations, precision));
    }

    // Load our power loads.
    gsNetwork->grids.at(0).nodes.at(7).s = -complex_t(0.004, 0.002);
    gsNetwork->grids.at(0).nodes.at(5).s = -complex_t(0.002, 0.001);
    gsNetwork->grids.at(0).nodes.at(6).s = -complex_t(0.005, 0.004);

    // Run all of our GS solvers.
    for (GridSolver *solver : gsSolvers)
    {
        solver->solve();
    }
    // ---------------------------------

    // ----- Setup ZBus Jacobi -----
    // Load our test file and make sure it exists.
    std::ifstream zBusTestFile(localPath + "examples/test_networks/test_network_cycle.txt");
    CHECK_FALSE(zBusTestFile.fail());

    // Create a loader that loads in the network from the file.
    NetworkLoader zBusLoader(zBusTestFile);
    std::unique_ptr<Network> zBusNetwork = zBusLoader.loadNetwork();

    // Add all our ZBus Jacobi solvers for each subnetwork.
    std::vector<GridSolver*> zBusSolvers;
    for (Grid &grid : zBusNetwork->grids)
    {
        zBusSolvers.push_back(new ZBusJacobiSolver(&grid, &logger, maxIterations, precision));
    }

    // Load our power loads.
    zBusNetwork->grids.at(0).nodes.at(7).s = -complex_t(0.004, 0.002);
    zBusNetwork->grids.at(0).nodes.at(5).s = -complex_t(0.002, 0.001);
    zBusNetwork->grids.at(0).nodes.at(6).s = -complex_t(0.005, 0.004);

    // Run all of our ZBus Jacobi solvers.
    for (GridSolver *solver : zBusSolvers)
    {
        solver->solve();
    }
    // -------------------------------

    // Check that GS and ZBus lay within the same precision of each other using Catch2 Matcher.
    CHECK_THAT(gsNetwork->grids[0].nodes[7].v.real(), Catch::Matchers::WithinAbs(zBusNetwork->grids[0].nodes[7].v.real(), 1e-10));
    CHECK_THAT(gsNetwork->grids[0].nodes[5].v.real(), Catch::Matchers::WithinAbs(zBusNetwork->grids[0].nodes[5].v.real(), 1e-10));
    CHECK_THAT(gsNetwork->grids[0].nodes[6].v.real(), Catch::Matchers::WithinAbs(zBusNetwork->grids[0].nodes[6].v.real(), 1e-10));

    CHECK_THAT(gsNetwork->grids[0].nodes[7].v.imag(), Catch::Matchers::WithinAbs(zBusNetwork->grids[0].nodes[7].v.imag(), 1e-10));
    CHECK_THAT(gsNetwork->grids[0].nodes[5].v.imag(), Catch::Matchers::WithinAbs(zBusNetwork->grids[0].nodes[5].v.imag(), 1e-10));
    CHECK_THAT(gsNetwork->grids[0].nodes[6].v.imag(), Catch::Matchers::WithinAbs(zBusNetwork->grids[0].nodes[6].v.imag(), 1e-10));
}

TEST_CASE("Compare treestructure", "[validation]")
{
    // ----- Common Setup -----
    SolverSettings settings{};
    CppLogger logger(std::cout);
    // ------------------------

    // ----- Setup test network with multiple grids -----
    // Load our test file and make sure it exists.
    std::ifstream multiFile(localPath + "examples/test_networks/test_network.txt");
    CHECK_FALSE(multiFile.fail());

    // Create a loader that loads in the network from the file.
    NetworkLoader multiGridLoader(multiFile);
    std::shared_ptr<Network> multiGridNetwork = multiGridLoader.loadNetwork();

    // Load our power (P) and voltage loads (V) for our multi grid network.
    std::vector<complex_t> multiP = {
        {0.002, 0.001},
        {0.005, 0.004},
        {0.004, 0.002}};
    std::vector<complex_t> multiV = {{1, 0}};

    PowerFlowSolver multiPfs(multiGridNetwork, settings, &logger);
    multiPfs.solve(multiP, multiV);
    // ---------------------------------

    // ----- Setup test network with single grid -----
    // Load our test file and make sure it exists.
    std::ifstream singleFile(localPath + "examples/test_networks/test_network_single_grid.txt");
    CHECK_FALSE(singleFile.fail());

    // Create a loader that loads in the network from the file.
    NetworkLoader singleLoader(singleFile);
    std::shared_ptr<Network> singleGridNetwork = singleLoader.loadNetwork();

    // Load our power (P) and voltage loads (V) for our single grid network.
    std::vector<complex_t> singleP = {
        {0.005, 0.004},
        {0.004, 0.002},
        {0.002, 0.001}};
    std::vector<complex_t> singleV = {{1, 0}};

    PowerFlowSolver singlePfs(singleGridNetwork, settings, &logger);
    singlePfs.solve(singleP, singleV);
    // ---------------------------------

    // Check that the single network and multi subnetwork lay within the same precision of each other using Catch2 Matcher,
    // ensuring that splitting the same single grid into multiple subgrids doesn't change the results.
    CHECK_THAT(singleGridNetwork->grids[0].nodes[1].v.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[0].nodes[1].v.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[2].v.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[0].nodes[2].v.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[3].v.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[0].nodes[3].v.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[4].v.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[1].nodes[1].v.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[5].v.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[2].nodes[1].v.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[6].v.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[2].nodes[2].v.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[7].v.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[1].nodes[2].v.real(), 1e-10));

    CHECK_THAT(singleGridNetwork->grids[0].nodes[1].v.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[0].nodes[1].v.imag(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[2].v.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[0].nodes[2].v.imag(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[4].v.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[1].nodes[1].v.imag(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[5].v.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[2].nodes[1].v.imag(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[6].v.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[2].nodes[2].v.imag(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[7].v.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[1].nodes[2].v.imag(), 1e-10));

    CHECK_THAT(singleGridNetwork->grids[0].nodes[1].s.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[0].nodes[1].s.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[2].s.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[0].nodes[2].s.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[3].s.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[0].nodes[3].s.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[4].s.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[1].nodes[1].s.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[5].s.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[2].nodes[1].s.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[6].s.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[2].nodes[2].s.real(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[7].s.real(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[1].nodes[2].s.real(), 1e-10));

    CHECK_THAT(singleGridNetwork->grids[0].nodes[1].s.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[0].nodes[1].s.imag(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[2].s.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[0].nodes[2].s.imag(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[3].s.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[0].nodes[3].s.imag(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[4].s.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[1].nodes[1].s.imag(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[5].s.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[2].nodes[1].s.imag(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[6].s.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[2].nodes[2].s.imag(), 1e-10));
    CHECK_THAT(singleGridNetwork->grids[0].nodes[7].s.imag(), Catch::Matchers::WithinAbs(multiGridNetwork->grids[1].nodes[2].s.imag(), 1e-10));
}

TEST_CASE("Choose solver", "[validation]")
{
    SECTION("GridAnalyzer")
    {
        // Load our test tree file and make sure it exists.
        std::ifstream treeFile(localPath + "examples/test_networks/test_network.txt");
        CHECK_FALSE(treeFile.fail());

        // Create a loader that loads in the tree network from the file.
        NetworkLoader treeLoader(treeFile);
        std::unique_ptr<Network> treeNetwork = treeLoader.loadNetwork();

        // Make sure that no subgrid has any cycle, i.e. all solvers shall have been
        // determined to be solveable by a Backward-Forward Sweep solver.
        GridAnalyzer analyzer;
        for (unsigned long gridIdx = 0; gridIdx < treeNetwork->grids.size(); gridIdx++)
        {
            REQUIRE(analyzer.determineSolver(treeNetwork->grids[gridIdx]) == BACKWARDFOWARDSWEEP);
        }

        // Load our test cycle file and make sure it exists.
        std::ifstream cycleFile(localPath + "examples/test_networks/test_network_cycle.txt");
        CHECK_FALSE(cycleFile.fail());

        // Create a loader that loads in the cycle network from the file.
        NetworkLoader cycleLoader(cycleFile);
        std::unique_ptr<Network> cycleNetwork = cycleLoader.loadNetwork();

        // Make sure we have at least one cycle in our subgrids, i.e. at least one
        // of our solvers shall have been determined to be either GaussSeidel or ZBusJacobi solver.
        bool containsCycle = false;
        for (unsigned long gridIdx = 0; gridIdx < cycleNetwork->grids.size(); gridIdx++)
        {
            if (analyzer.determineSolver(cycleNetwork->grids[gridIdx]) != BACKWARDFOWARDSWEEP)
            {
                containsCycle = true;
            }
        }
        REQUIRE(containsCycle);

        // Load our multiple slack nodes file and make sure it exists.
        std::ifstream slackFile(localPath + "examples/test_networks/multiple_slack_nodes.txt");
        CHECK_FALSE(slackFile.fail());

        // Create a loader that loads in the slack network from the file.
        NetworkLoader slackLoader(slackFile);
        std::unique_ptr<Network> slackNetwork = slackLoader.loadNetwork();

        // Make sure GaussSeidel is determined as solver when we have multiple slack nodes,
        // since Backward-Forward Sweep (can only handle one slack node) and ZBusJacobi (in
        // its current implementation cannot handle this -- the math is much more complex).
        SolverType solverTypeSlack = analyzer.determineSolver(slackNetwork->grids[2]);
        REQUIRE(solverTypeSlack != ZBUSJACOBI);
        REQUIRE(solverTypeSlack != BACKWARDFOWARDSWEEP);
    }
}
