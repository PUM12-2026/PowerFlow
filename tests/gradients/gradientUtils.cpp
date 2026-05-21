#include "gradientUtils.hpp"

GradientTestEnvironment setupEnvironment(const std::string &filename, Logger *logger, bool computeGradients)
{
    std::ifstream testFile(filename);
    REQUIRE_FALSE(testFile.fail());

    NetworkLoader loader(testFile);
    std::shared_ptr<Network> network = loader.loadNetwork();

    SolverSettings settings{};
    settings.bfs_precision = PRECISION;
    settings.max_iterations_bfs = MAX_ITERATIONS;
    settings.compute_gradients = computeGradients;

    static CppLogger defaultLogger(std::cout);
    Logger *activeLogger = (logger != nullptr) ? logger : &defaultLogger;

    std::unique_ptr<PowerFlowSolver> pfs = std::make_unique<PowerFlowSolver>(network, settings, activeLogger);

    return {std::move(network), std::move(pfs)};
}

std::vector<std::pair<size_t, node_idx_t>> getNetworkLeafNodeIndices(const Network &network)
{
    std::vector<std::pair<size_t, node_idx_t>> leafNodeIndices;

    for (size_t gridIdx = 0; gridIdx < network.grids.size(); ++gridIdx)
    {
        const Grid &grid = network.grids[gridIdx];
        for (size_t nIdx = 0; nIdx < grid.nodes.size(); ++nIdx)
        {
            if (grid.nodes[nIdx].type == NodeType::LOAD)
            {
                leafNodeIndices.push_back({gridIdx, static_cast<node_idx_t>(nIdx)});
            }
        }
    }

    return leafNodeIndices;
}

std::vector<edge_idx_t> mapUpstreamEdges(const Grid &grid)
{
    size_t nodeCount = grid.nodes.size();
    std::vector<edge_idx_t> upstreamEdgeMap(nodeCount, -1);
    std::vector<size_t> parentOf(nodeCount, -1);
    std::queue<size_t> traversalQueue;

    traversalQueue.push(ROOT_NODE_ID);

    // Mark root as visited
    parentOf[ROOT_NODE_ID] = ROOT_NODE_ID;

    while (!traversalQueue.empty())
    {
        size_t currentNode = traversalQueue.front();
        traversalQueue.pop();

        for (const auto &edgeIdx : grid.nodes[currentNode].edges)
        {
            const auto &edge = grid.edges[edgeIdx];
            size_t neighborNode = (edge.parent == static_cast<node_idx_t>(currentNode)) ? edge.child : edge.parent;

            if (parentOf[neighborNode] == static_cast<size_t>(-1))
            {
                parentOf[neighborNode] = currentNode;
                upstreamEdgeMap[neighborNode] = edgeIdx;
                traversalQueue.push(neighborNode);
            }
        }
    }

    return upstreamEdgeMap;
}

NodeState captureCurrentNetworkState(const Grid &grid)
{
    NodeState currentState;
    size_t nodeCount = grid.nodes.size();

    std::vector<edge_idx_t> upstreamEdgeMap = mapUpstreamEdges(grid);

    currentState.v.assign(nodeCount, 0.0);
    currentState.s.assign(nodeCount, 0.0);
    currentState.i.assign(nodeCount, 0.0);
    currentState.sloss.assign(nodeCount, 0.0);

    for (size_t i = 0; i < nodeCount; ++i)
    {
        currentState.v[i] = grid.nodes[i].v;
        currentState.s[i] = grid.nodes[i].s;

        edge_idx_t feedingEdgeIdx = upstreamEdgeMap[i];
        if (feedingEdgeIdx != -1)
        {
            const GridEdge &edge = grid.edges[feedingEdgeIdx];
            const GridNode &p = grid.nodes[edge.parent];
            const GridNode &c = grid.nodes[edge.child];

            // Avoid division by zero
            complex_t impedance = (edge.z_c != 0.0) ? edge.z_c : static_cast<complex_t>(1e-12);
            complex_t currentInBranch = (p.v - c.v) / (impedance * SQRT3);

            currentState.i[i] = currentInBranch;
            currentState.sloss[i] = 3.0 * edge.z_c * currentInBranch * std::conj(currentInBranch);
        }
    }

    return currentState;
}

std::pair<double, double> calculateCentralDifference(
    const complex_t &plusP, const complex_t &minusP,
    const complex_t &plusQ, const complex_t &minusQ //,
)
{
    double dPre = (plusP.real() - minusP.real()) / (2.0 * DELTA);
    double dQim = (plusQ.imag() - minusQ.imag()) / (2.0 * DELTA);
    return {dPre, dQim};
}

NumericGradients computeNumericGradients(
    PowerFlowSolver &solver,
    const Grid &grid,
    std::vector<complex_t> &S,
    std::vector<complex_t> &V,
    size_t leafIdx // ,
)
{
    NumericGradients numericGradients;
    size_t nodeCount = grid.nodes.size();

    numericGradients.dVdS.resize(nodeCount);
    numericGradients.dSdS.resize(nodeCount);
    numericGradients.dIdS.resize(nodeCount);
    numericGradients.dSlossdS.resize(nodeCount);

    complex_t originalS = S[leafIdx];

    // S_P + delta
    S[leafIdx] = originalS + complex_t(DELTA, 0);
    solver.solve(S, V);
    NodeState plusP = captureCurrentNetworkState(grid);

    // S_P - delta
    S[leafIdx] = originalS - complex_t(DELTA, 0);
    solver.solve(S, V);
    NodeState minusP = captureCurrentNetworkState(grid);

    // S_Q + delta
    S[leafIdx] = originalS + complex_t(0, DELTA);
    solver.solve(S, V);
    NodeState plusQ = captureCurrentNetworkState(grid);

    // S_Q - delta
    S[leafIdx] = originalS - complex_t(0, DELTA);
    solver.solve(S, V);
    NodeState minusQ = captureCurrentNetworkState(grid);

    // Restore original power injection
    S[leafIdx] = originalS;
    solver.solve(S, V);

    for (size_t i = 0; i < nodeCount; ++i)
    {
        numericGradients.dVdS[i] = calculateCentralDifference(plusP.v[i], minusP.v[i], plusQ.v[i], minusQ.v[i]);
        numericGradients.dSdS[i] = calculateCentralDifference(plusP.s[i], minusP.s[i], plusQ.s[i], minusQ.s[i]);
        numericGradients.dIdS[i] = calculateCentralDifference(plusP.i[i], minusP.i[i], plusQ.i[i], minusQ.i[i]);
        numericGradients.dSlossdS[i] = calculateCentralDifference(plusP.sloss[i], minusP.sloss[i], plusQ.sloss[i], minusQ.sloss[i]);
    }

    return numericGradients;
}

void verifyAnalyticGradients(
    int index,
    size_t gridIdx,
    size_t leafGridIdx,
    size_t leafNodeIdx,
    const Eigen::Matrix2d &analytic,
    std::pair<double, double> numeric,
    const char *label,
    bool isEdge // just here to ease the print
)
{
    std::cout << "[" << label << "] " << (isEdge ? "Edge " : "Node ") << index << " (Grid " << gridIdx
              << ") to Leaf (Grid " << leafGridIdx << ", Node " << leafNodeIdx << ")"
              << ": Analytic(" << analytic(0, 0) << "," << analytic(1, 1) << ") Numeric("
              << numeric.first << "," << numeric.second << ")" << std::endl;

    CHECK_THAT(analytic(0, 0), Catch::Matchers::WithinAbs(numeric.first, TEST_TOLERANCE));
    CHECK_THAT(analytic(1, 1), Catch::Matchers::WithinAbs(numeric.second, TEST_TOLERANCE));
}

void runGradientTests(const GradientTestEnvironment &environment, size_t gridIdx, std::vector<complex_t> S, std::vector<complex_t> V)
{
    const Grid &grid = environment.network->grids[gridIdx];
    std::vector<edge_idx_t> upstreamEdgeMap = mapUpstreamEdges(grid);

    std::vector<std::pair<size_t, node_idx_t>> networkLeafNodes = getNetworkLeafNodeIndices(*environment.network);
    std::vector<NumericGradients> allNumericGradients(networkLeafNodes.size());

    for (size_t leafIdx = 0; leafIdx < networkLeafNodes.size(); ++leafIdx)
    {
        allNumericGradients[leafIdx] = computeNumericGradients(*environment.pfs, grid, S, V, leafIdx);
    }

    DYNAMIC_SECTION("Compare dVdS gradients with numerical approximation gradients - Grid " << gridIdx)
    {
        for (size_t leafIdx = 0; leafIdx < networkLeafNodes.size(); ++leafIdx)
        {
            for (size_t nodeIdx = 0; nodeIdx < grid.nodes.size(); ++nodeIdx)
            {
                verifyAnalyticGradients(
                    nodeIdx,
                    gridIdx,
                    networkLeafNodes[leafIdx].first,
                    networkLeafNodes[leafIdx].second,
                    grid.dVdS[nodeIdx][leafIdx],
                    allNumericGradients[leafIdx].dVdS[nodeIdx],
                    "dVdS" // ,
                );
            }
        }
    }

    DYNAMIC_SECTION("Compare dSdS gradients with numerical approximation gradients - Grid " << gridIdx)
    {
        for (size_t leafIdx = 0; leafIdx < networkLeafNodes.size(); ++leafIdx)
        {
            for (size_t nodeIdx = 0; nodeIdx < grid.nodes.size(); ++nodeIdx)
            {
                verifyAnalyticGradients(
                    nodeIdx,
                    gridIdx,
                    networkLeafNodes[leafIdx].first,
                    networkLeafNodes[leafIdx].second,
                    grid.dSdS[nodeIdx][leafIdx],
                    allNumericGradients[leafIdx].dSdS[nodeIdx],
                    "dSdS" // ,
                );
            }
        }
    }

    DYNAMIC_SECTION("Compare dIdS gradients with numerical approximation gradients - Grid " << gridIdx)
    {
        for (size_t leafIdx = 0; leafIdx < networkLeafNodes.size(); ++leafIdx)
        {
            for (size_t nodeIdx = 0; nodeIdx < grid.nodes.size(); ++nodeIdx)
            {
                edge_idx_t edgeIdx = upstreamEdgeMap[nodeIdx];
                Eigen::Matrix2d analyticDidS = (edgeIdx == -1) ? Eigen::Matrix2d::Zero() : grid.dIdS[edgeIdx][leafIdx];

                verifyAnalyticGradients(
                    edgeIdx,
                    gridIdx,
                    networkLeafNodes[leafIdx].first,
                    networkLeafNodes[leafIdx].second,
                    analyticDidS,
                    allNumericGradients[leafIdx].dIdS[nodeIdx],
                    "dIdS",
                    true // ,
                );
            }
        }
    }

    DYNAMIC_SECTION("Compare dSlossdS gradients with numerical approximation gradients - Grid " << gridIdx)
    {
        for (size_t leafIdx = 0; leafIdx < networkLeafNodes.size(); ++leafIdx)
        {
            for (size_t nodeIdx = 0; nodeIdx < grid.nodes.size(); ++nodeIdx)
            {
                edge_idx_t edgeIdx = upstreamEdgeMap[nodeIdx];
                Eigen::Matrix2d analyticDslossdS = (edgeIdx == -1) ? Eigen::Matrix2d::Zero() : grid.dSlossdS[edgeIdx][leafIdx];

                verifyAnalyticGradients(
                    edgeIdx,
                    gridIdx,
                    networkLeafNodes[leafIdx].first,
                    networkLeafNodes[leafIdx].second,
                    analyticDslossdS,
                    allNumericGradients[leafIdx].dSlossdS[nodeIdx],
                    "dSlossdS",
                    true // ,
                );
            }
        }
    }
}
