#include "powerflow/solvers/BackwardForwardSweepSolver.hpp"
#include "powerflow/computers/GradientComputer.hpp"
#include "powerflow/network.hpp"

#include <algorithm>
#include <queue>

BackwardForwardSweepSolver::BackwardForwardSweepSolver(
    Grid *grid, Logger *const logger, int maxIter, double precision, bool computeGradients,
    size_t networkLeafCount,
    const std::vector<size_t> &gridLeafIndicies // ,
    ) : GridSolver(grid, logger, maxIter, precision), networkLeafCount(networkLeafCount),
        gridLeafIndicies(gridLeafIndicies), computeGradients(computeGradients)
{
    rootIdx = -1;
    for (size_t i = 0; i < grid->nodes.size(); ++i)
    {
        // Finds the index of root node.
        if (grid->nodes[i].type == NodeType::SLACK_IMPLICIT || grid->nodes[i].type == NodeType::SLACK)
        {
            rootIdx = i;
            break;
        }
    }

    if (rootIdx == -1)
    {
        throw std::runtime_error("BackwardForwardSweepSolver: Could not find index of root node");
    }

    I.resize(grid->edges.size(), 0.0);

    previousVoltages.resize(grid->nodes.size(), {0, 0});
}

int BackwardForwardSweepSolver::solve()
{
    if (!firstRun && hasConverged())
    {
        return 0;
    }

    firstRun = false;

    int iter = 0;

    if (grid->dIdS.empty() && computeGradients)
    {
        // Sizing of the gradient matrices based on total leaf nodes in the network.
        grid->dIdS.resize(grid->edges.size());
        grid->dVdS.resize(grid->nodes.size());
        grid->dSdS.resize(grid->nodes.size());
        grid->dSlossdS.resize(grid->edges.size());

        for (size_t nodeIdx = 0; nodeIdx < grid->nodes.size(); ++nodeIdx)
        {
            grid->dVdS[nodeIdx].assign(networkLeafCount, Eigen::Matrix2d::Zero());
            grid->dSdS[nodeIdx].assign(networkLeafCount, Eigen::Matrix2d::Zero());
        }

        for (size_t edgeIdx = 0; edgeIdx < grid->edges.size(); ++edgeIdx)
        {
            grid->dIdS[edgeIdx].assign(networkLeafCount, Eigen::Matrix2d::Zero());
            grid->dSlossdS[edgeIdx].assign(networkLeafCount, Eigen::Matrix2d::Zero());
        }

        // Initialize dS/dS for local leaf nodes (households and implicit connections)
        size_t leafCounter = 0;
        for (size_t localIdx = 0; localIdx < grid->nodes.size(); ++localIdx)
        {
            if (grid->nodes[localIdx].type == NodeType::LOAD)
            {
                size_t globalIdx = gridLeafIndicies[leafCounter++];
                grid->dSdS[localIdx][globalIdx] = Eigen::Matrix2d::Identity();
            }
        }
    }

    // Update previous voltage vector and run forward-backward sweeps until convergence or max iterations
    do
    {
        for (size_t i = 0; i < grid->nodes.size(); i++)
        {
            previousVoltages[i] = grid->nodes[i].v;
        }

        sweep(rootIdx, -1);
    }
    while (++iter < maxIterations);// && !hasConverged());

    return iter;
}

complex_t BackwardForwardSweepSolver::sweep(node_idx_t nodeIdx, edge_idx_t prevEdgeIdx)
{
    GridNode &node = grid->nodes[nodeIdx];

    bool isRoot = prevEdgeIdx == -1;
    bool isLeaf = !isRoot && node.edges.size() == 1;

    // Forward sweep
    if (!isRoot)
    {
        GridEdge &prevEdge = grid->edges[prevEdgeIdx];
        node_idx_t prevNodeIdx = prevEdge.parent == nodeIdx ? prevEdge.child : prevEdge.parent;
        GridNode &prevNode = grid->nodes[prevNodeIdx];

        I[prevEdgeIdx] = std::conj((node.s) / (SQRT3 * node.v));

        // Compute dI/dS for the cables downstream of this node
        if (computeGradients)
        {
            grid->dIdS[prevEdgeIdx] = gradientComputer.computeDiDs(node.v, node.s, grid->dSdS[nodeIdx], grid->dVdS[nodeIdx]);
        }

        // Forward propagation of voltage
        node.v = prevNode.v - SQRT3 * I[prevEdgeIdx] * prevEdge.z_c;

        // Compute dV/dS for all households downstream of this node
        if (computeGradients)
        {
            grid->dVdS[nodeIdx] = gradientComputer.computeDvDs(
                grid->dVdS[prevNodeIdx],
                grid->dIdS[prevEdgeIdx],
                prevEdge.z_c,
                networkLeafCount //,
            );
        }
    }

    complex_t s = 0;

    // Initialize dS/dS for non-leaf nodes to zero, as they do not have any power.
    if (!isLeaf && computeGradients)
    {
        for (size_t i = 0; i < networkLeafCount; ++i)
        {
            grid->dSdS[nodeIdx][i].setZero();
        }
    }

    for (node_idx_t edgeIdx : node.edges)
    {
        if (edgeIdx == prevEdgeIdx)
            continue;

        GridEdge &edge = grid->edges[edgeIdx];
        node_idx_t nextIdx = edge.parent == nodeIdx ? edge.child : edge.parent;

        // Backward sweep
        s += sweep(nextIdx, edgeIdx);

        // Compute power loss sensitivity for this cable
        if (computeGradients)
        {
            grid->dSlossdS[edgeIdx] = gradientComputer.computeDsLossDs(
                grid->nodes[nextIdx].v,
                I[edgeIdx],
                edge.z_c,
                grid->dVdS[nextIdx],
                grid->dIdS[edgeIdx],
                networkLeafCount // ,
            );

            // Compute the power sensitivity for this node against leaf nodes
            std::vector<Eigen::Matrix2d> dSdS = gradientComputer.computeDsDs(
                grid->dSdS[nextIdx],
                grid->dSlossdS[edgeIdx] // ,
            );

            for (size_t i = 0; i < networkLeafCount; ++i)
            {
                grid->dSdS[nodeIdx][i] += dSdS[i];
            }
        }
    }

    if (!isLeaf)
    {
        node.s = s;
    }

    if (!isRoot)
    {
        GridEdge &prevEdge = grid->edges[prevEdgeIdx];
        complex_t lineLoss = 3.0 * prevEdge.z_c * I[prevEdgeIdx] * std::conj(I[prevEdgeIdx]);
        complex_t upstreamPower = node.s + lineLoss;
        return upstreamPower;
    }

    return (0.0);
}

void BackwardForwardSweepSolver::reset()
{
    GridSolver::reset();
    std::fill(I.begin(), I.end(), 0);

    if (computeGradients)
    {
        // Make sure to clear the gradient matricies.
        grid->dIdS.clear();
        grid->dSlossdS.clear();
        grid->dIdS.clear();
        grid->dSdS.clear();
        grid->dVdS.clear();
    }

    firstRun = true;
}

bool BackwardForwardSweepSolver::hasConverged()
{
    for (size_t i = 0; i < grid->nodes.size(); i++)
    {
        if (std::abs(grid->nodes[i].v - previousVoltages[i]) > precision) return false;
    }
    return true;
}
