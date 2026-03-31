#include "powerflow/solvers/BackwardForwardSweepSolver.hpp"
#include "powerflow/network.hpp"

#include <algorithm>

BackwardForwardSweepSolver::BackwardForwardSweepSolver(Grid* grid,
    Logger* const logger, int maxIter, double precision)
    : GridSolver(grid, logger, maxIter, precision)
{
    rootIdx = -1;
    for (node_idx_t i = 0; i < grid->nodes.size(); ++i)
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
}

int BackwardForwardSweepSolver::solve()
{
    if (!firstRun && hasConverged())
    {
        return 0;
    }

    firstRun = false;

    int iter = 0;
    bool converged = false;

    // SLACK power must be negative in the BFS algorithm.
    grid->nodes[rootIdx].s = -grid->nodes[rootIdx].s;

    // Updates the complex power injection.
    while (!converged && iter++ < maxIterations)
    {
        sweep(rootIdx, -1);
        converged = hasConverged();
    }

    // Change SLACK power to positive value before returning.
    grid->nodes[rootIdx].s = -grid->nodes[rootIdx].s;

    if (!converged)
    {
        throw std::runtime_error("BackwardForwardSweepSolver: The solution did not converge. Maximum number of iterations reached.");
    }
    return iter;
}

complex_t BackwardForwardSweepSolver::sweep(node_idx_t nodeIdx,
    edge_idx_t prevEdgeIdx)
{
    GridNode& node = grid->nodes[nodeIdx];

    bool isRoot = prevEdgeIdx == -1;

    if (!isRoot)
    {
        GridEdge& prevEdge = grid->edges[prevEdgeIdx];
        node_idx_t prevNodeIdx = prevEdge.parent == nodeIdx ? prevEdge.child : prevEdge.parent;
        GridNode& prevNode = grid->nodes[prevNodeIdx];

        I[prevEdgeIdx] = std::conj((-node.s) / (SQRT3 * node.v));
        node.v = prevNode.v - SQRT3 * I[prevEdgeIdx] * prevEdge.z_c;
    }

    complex_t s = 0;

    for (node_idx_t edgeIdx : node.edges)
    {
        if (edgeIdx == prevEdgeIdx)
            continue;

        GridEdge& edge = grid->edges[edgeIdx];
        node_idx_t nextIdx = edge.parent == nodeIdx ? edge.child : edge.parent;

        s += sweep(nextIdx, edgeIdx);
    }

    bool isLeaf = !isRoot && node.edges.size() == 1;

    if (!isLeaf)
    {
        node.s = s;
    }

    if (!isRoot)
    {
        GridEdge& prevEdge = grid->edges[prevEdgeIdx];

        return node.s - 3.0 * prevEdge.z_c * I[prevEdgeIdx] *
            std::conj(I[prevEdgeIdx]);
    }
    return (0.0);
}

void BackwardForwardSweepSolver::reset()
{
    GridSolver::reset();
    std::fill(I.begin(), I.end(), 0);
    firstRun = true;
}

bool BackwardForwardSweepSolver::hasConverged()
{
    for (node_idx_t nodeIdx = 0; nodeIdx < grid->nodes.size(); ++nodeIdx)
    {
        GridNode& node = grid->nodes[nodeIdx];

        if (node.type == NodeType::SLACK_IMPLICIT || node.type == NodeType::SLACK)
            continue;

        // Complex current going through the node.
        complex_t yv = 0.0;

        // Sum of neighbour admittances.
        complex_t ySum = 0.0;

        for (size_t edgeIdx : node.edges)
        {
            GridEdge& edge = grid->edges[edgeIdx];
            node_idx_t neighborIdx = edge.parent == nodeIdx ? edge.child : edge.parent;
            GridNode& neighbor = grid->nodes[neighborIdx];

            complex_t y = 1.0 / edge.z_c;
            yv -= neighbor.v * y;
            ySum += y;
        }
        // Update current
        yv += node.v * ySum;

        bool isLeaf = node.edges.size() == 1;

        if (isLeaf)
        {
            if (std::abs(node.v * std::conj(yv) - node.s) > precision)
            {
                return false;
            }
        }
        else
        {
            if (std::abs(node.v * std::conj(yv) - 0.0) > precision)
            {
                return false;
            }
        }
    }
    return true;
}
