#include "powerflow/PowerFlowSolver.hpp"
#include "powerflow/solvers/GaussSeidelSolver.hpp"
#include "powerflow/solvers/ZBusJacobiSolver.hpp"
#include "powerflow/solvers/BackwardForwardSweepSolver.hpp"
#include "powerflow/SolverTypeEnum.hpp"
#include "powerflow/GridAnalyzer.hpp"
#include "powerflow/NetworkValidator.hpp"
#include "powerflow/NetworkSave.hpp"

#include <cmath>
#include <iostream>

// Constructor initilizes solver for a given network
// Initilizes and validates network and settings
PowerFlowSolver::PowerFlowSolver(std::shared_ptr<Network> network, SolverSettings settings, Logger *const logger) : network{network}, settings{std::move(settings)}, logger{logger}
{
    if (settings.max_iterations_total <= 0)
    {
        throw std::invalid_argument("Invalid max_iterations_total value");
    }
    if (settings.max_iterations_gauss <= 0)
    {
        throw std::invalid_argument("Invalid max_iterations_gauss value");
    }
    if (settings.gauss_seidel_precision <= 0)
    {
        throw std::invalid_argument("Invalid gauss_seidel_precision value");
    }
    if (settings.max_iterations_bfs <= 0)
    {
        throw std::invalid_argument("Invalid max_iterations_bfs value");
    }
    if (settings.bfs_precision <= 0)
    {
        throw std::invalid_argument("Invalid bfs_precision value");
    }
    if (settings.max_iterations_zbusjacobi <= 0)
    {
        throw std::invalid_argument("Invalid max_iterations_zbusjacobi value");
    }
    if (settings.zbusjacobi_precision <= 0)
    {
        throw std::invalid_argument("Invalid zbus_jacobi_precision value");
    }

    NetworkValidator validator;
    validator.validateNetwork(*network);
}

// Entry point for solving network
void PowerFlowSolver::solve(const std::vector<complex_t> &S, const std::vector<complex_t> &V)
{
    if (firstRun)
    {
        createGridSolvers();
        firstRun = false;
    }

    // Update network state
    updateLoads(S);
    updateExternalVoltages(V);
    runGridSolvers();
}

void PowerFlowSolver::solveParams(const std::unordered_map<node_idx_t, complex_t> &V, double precision)
{
    for (Grid& grid : network->grids)
    {
        ParameterValidator pv = ParameterValidator(&grid, logger, &settings, V, precision);
        pv.validate();
    }
}

std::vector<complex_t> PowerFlowSolver::solveParamsOLS(std::unordered_map<node_idx_t, MeasuredValues> &measuredValues, 
    std::vector<complex_t> &slackVoltages)
{
    ParameterValidator pv = ParameterValidator(&network->grids[0], logger, &settings, {}, 1e9);
    return pv.validateRegression(measuredValues, slackVoltages);
}

std::vector<complex_t> PowerFlowSolver::solveParamsLAD(std::unordered_map<node_idx_t, MeasuredValues> &measuredValues, 
    std::vector<complex_t> &slackVoltages)
{
    ParameterValidator pv = ParameterValidator(&network->grids[0], logger, &settings, {}, 1e9);
    return pv.validateLAD(measuredValues, slackVoltages);   
}

// Creates appropriate GridSolvers for each grid in the network
// Depending on the characteristics of the grid
void PowerFlowSolver::createGridSolvers()
{
    GridAnalyzer analyzer;
    int gridNo = 0;

    size_t totalLeafCount = 0;

    // Collect total leaf count from all grids in the network.
    for (const Grid &grid : network->grids)
    {
        for (size_t nodeIdx = 0; nodeIdx < grid.nodes.size(); ++nodeIdx)
        {
            if (grid.nodes[nodeIdx].type == NodeType::LOAD)
            {
                totalLeafCount++;
            }
        }
    }

    size_t networkLeafIdx = 0;
    for (Grid &grid : network->grids)
    {
        // Collect leaf indices for this grid, which are needed for BFS.
        std::vector<size_t> gridLeafIndicies;
        for (size_t nodeIdx = 0; nodeIdx < grid.nodes.size(); ++nodeIdx)
        {
            if (grid.nodes[nodeIdx].type == NodeType::LOAD)
            {
                gridLeafIndicies.push_back(networkLeafIdx++);
            }
        }

        // Decide which solver algorithm fits the grid structure
        switch (analyzer.determineSolver(grid))
        {
        case GAUSSSEIDEL:
        {
            *logger << "Found grid number " << gridNo << " suitable for Gauss-Seidel" << std::endl;

            if (settings.compute_gradients)
            {
                *logger << "Gradients will not be computed." << std::endl;
                settings.compute_gradients = false;
            }

            std::unique_ptr<GaussSeidelSolver> gs = std::make_unique<GaussSeidelSolver>(
                &grid, logger,
                settings.max_iterations_gauss,
                settings.gauss_seidel_precision // ,
            );
            gridSolvers.push_back(std::move(gs));
            break;
        }
        case BACKWARDFOWARDSWEEP:
        {
            *logger << "Found grid number " << gridNo << " suitable for BFS" << std::endl;
            std::unique_ptr<BackwardForwardSweepSolver> bfs = std::make_unique<BackwardForwardSweepSolver>(
                &grid, logger,
                settings.max_iterations_bfs,
                settings.bfs_precision,
                settings.compute_gradients,
                totalLeafCount,
                gridLeafIndicies // ,
            );
            gridSolvers.push_back(std::move(bfs));
            break;
        }
        case ZBUSJACOBI:
        {
            *logger << "Found grid number " << gridNo << " suitable for ZBus Jacobi" << std::endl;

            if (settings.compute_gradients)
            {
                *logger << "Gradients will not be computed." << std::endl;
                settings.compute_gradients = false;
            }

            std::unique_ptr<ZBusJacobiSolver> bfs = std::make_unique<ZBusJacobiSolver>(
                &grid, logger,
                settings.max_iterations_zbusjacobi,
                settings.zbusjacobi_precision // ,
            );
            gridSolvers.push_back(std::move(bfs));
            break;
        }
        default:
            throw std::runtime_error("No suitable solver found for grid number " + std::to_string(gridNo));
        }
        ++gridNo;
    }
}

// Updates LOAD node powers
void PowerFlowSolver::updateLoads(const std::vector<complex_t> &S)
{
    size_t powerIdx = 0;

    for (Grid &grid : network->grids)
    {
        for (GridNode &node : grid.nodes)
        {
            if (node.type == NodeType::LOAD)
            {
                if (powerIdx == S.size())
                {
                    throw std::runtime_error("S has too few elements");
                }
                node.s = S.at(powerIdx++);
            }
        }
    }
    if (powerIdx != S.size())
    {
        throw std::runtime_error("S is of incorrect size");
    }
}

// Updates SLACK node voltages
void PowerFlowSolver::updateExternalVoltages(const std::vector<complex_t> &V)
{
    size_t voltageIdx = 0;

    for (Grid &grid : network->grids)
    {
        for (GridNode &node : grid.nodes)
        {
            if (node.type == NodeType::SLACK)
            {
                if (voltageIdx == V.size())
                {
                    throw std::runtime_error("V has too few elements");
                }
                node.v = V.at(voltageIdx++);
            }
        }
    }
    if (voltageIdx != V.size())
    {
        throw std::runtime_error("V is of incorrect size");
    }
}

// Runs the GridSolvers and combines the result
void PowerFlowSolver::runGridSolvers()
{
    int iteration = 0;
    int maxGridIteration = 0;

    do
    {
        maxGridIteration = 0;

        // Solve each grid independently
        for (std::unique_ptr<GridSolver> &solver : gridSolvers)
        {
            int gridIter = solver->solve();

            // Track worst convergence
            maxGridIteration = std::max(gridIter, maxGridIteration);
        }

        // Update connections between grids
        // Simulates "fake" connection with z = 0
        for (GridConnection &connection : network->connections)
        {
            Grid &loadImplicitGrid = network->grids[connection.loadImplicitGrid];
            Grid &slackImplicitGrid = network->grids[connection.slackImplicitGrid];
            GridNode &loadImplicitNode = loadImplicitGrid.nodes[connection.loadImplicitNode];
            GridNode &slackImplicitNode = slackImplicitGrid.nodes[connection.slackImplicitNode];

            // Transfer power between grids
            loadImplicitNode.s = ((slackImplicitNode.s * slackImplicitGrid.sBase) / loadImplicitGrid.sBase);

            // Transfer sensitivities for dS/dS between grids, after having updated power.
            if (!loadImplicitGrid.dSdS.empty() && !slackImplicitGrid.dSdS.empty())
            {
                for (size_t i = 0; i < loadImplicitGrid.dSdS[connection.loadImplicitNode].size(); ++i)
                {
                    auto dSdS = slackImplicitGrid.dSdS[connection.slackImplicitNode][i];
                    auto normalizedDsdS = dSdS * (slackImplicitGrid.sBase / loadImplicitGrid.sBase);
                    loadImplicitGrid.dSdS[connection.loadImplicitNode][i] = normalizedDsdS;
                }
            }

            // Enforce equal voltage
            slackImplicitNode.v = loadImplicitNode.v;

            // Transfer sensitivities for dV/dS between grids, after having updated voltage.
            if (!loadImplicitGrid.dVdS.empty() && !slackImplicitGrid.dVdS.empty())
            {
                for (size_t i = 0; i < slackImplicitGrid.dVdS[connection.slackImplicitNode].size(); ++i)
                {
                    auto dVdS = loadImplicitGrid.dVdS[connection.loadImplicitNode][i];
                    slackImplicitGrid.dVdS[connection.slackImplicitNode][i] = dVdS;
                }
            }
        }

        iteration++;
    } while (maxGridIteration > 0 && iteration < settings.max_iterations_total);

    // If solution hasnt converged after max number of iterations
    if (maxGridIteration > 0)
    {
        *logger << "[PowerFlow] Solution did not converge, max number of iterations reached." << std::endl;
        //throw std::runtime_error("PowerFlowSolver: The solution did not converge. Maximum number of iterations reached.");
    }
}

// Returns all LOAD voltages in the network
std::vector<complex_t> PowerFlowSolver::getLoadVoltages() const
{
    std::vector<complex_t> U;

    for (Grid const &grid : network->grids)
    {
        for (GridNode const &node : grid.nodes)
        {
            if (node.type == NodeType::LOAD)
            {
                U.push_back(node.v);
            }
        }
    }
    return U;
}

// Returns all voltages in the network
std::vector<complex_t> PowerFlowSolver::getAllVoltages() const
{
    std::vector<complex_t> result{};

    for (Grid const &grid : network->grids)
    {
        for (GridNode const &node : grid.nodes)
        {
            result.push_back(node.v);
        }
    }
    return result;
}

// Returns all currents in the network
std::vector<complex_t> PowerFlowSolver::getCurrents() const
{
    std::vector<complex_t> result{};

    for (Grid const &grid : network->grids)
    {
        for (GridEdge const &edge : grid.edges)
        {
            GridNode parent{grid.nodes[edge.parent]}, child{grid.nodes[edge.child]};

            // Avoid division by zero if impedance zero substitute by small number
            complex_t impedance = (edge.z_c != 0.0) ? edge.z_c : static_cast<complex_t>(settings.gauss_seidel_precision);
            complex_t current{(parent.v - child.v) / (impedance * SQRT3)};
            result.push_back(current);
        }
    }
    return result;
}

// Returns all SLACK_IMPLICIT/SLACK powers in the network
std::vector<complex_t> PowerFlowSolver::getSlackPowers() const
{
    std::vector<complex_t> result{};

    for (Grid const &grid : network->grids)
    {
        for (GridNode const &node : grid.nodes)
        {
            if (node.type == NodeType::SLACK_IMPLICIT || node.type == NodeType::SLACK)
            {
                result.push_back(node.s);
            }
        }
    }

    return result;
}

std::vector<complex_t> PowerFlowSolver::getImpedances() const
{
    std::vector<complex_t> result;
    
    for (Grid const &g : network->grids)
    {
        for (GridEdge const &e : g.edges)
        {
            result.push_back(e.z_c);
        }
    }

    return result;
}

void PowerFlowSolver::setImpedances(std::vector<complex_t> &Z)
{
    size_t i = 0;
    for (Grid &grid : network->grids)
    {
        for (GridEdge &edge : grid.edges)
        {
            if (i >= Z.size())
            {
                throw std::runtime_error("Z has too few elements");
            }

            edge.z_c = Z[i];
            i++;
        }
    }
}

std::vector<std::vector<std::array<double, 2>>> PowerFlowSolver::getDvDs() const
{
    if (!settings.compute_gradients)
    {
        throw std::runtime_error("Gradients are unavailable. Enable 'compute_gradients' in settings and ensure the network uses Backward Forward Sweep.");
    }

    size_t rootIdx = -1;
    for (Grid const &grid : network->grids)
    {
        for (size_t nodeIdx = 0; nodeIdx < grid.nodes.size(); ++nodeIdx)
        {
            // Finds the index of root node.
            if (grid.nodes[nodeIdx].type == NodeType::SLACK)
            {
                rootIdx = nodeIdx;
                break;
            }
        }
    }

    std::vector<std::vector<std::array<double, 2>>> result;

    for (size_t gridIdx = 0; gridIdx < network->grids.size(); ++gridIdx)
    {
        Grid const &grid = network->grids[gridIdx];
        for (size_t nodeIdx = 0; nodeIdx < grid.dVdS.size(); ++nodeIdx)
        {
            // Skip the root node of the grid, i.e. the SLACK node, since it has no dV/dS sensitivities (dVdS=[0,0,0]).
            if (gridIdx == 0 && nodeIdx == rootIdx)
            {
                continue;
            }

            std::vector<std::array<double, 2>> dVNode_dSLeaves;

            for (size_t leafIdx = 0; leafIdx < grid.dVdS[nodeIdx].size(); ++leafIdx)
            {
                double dVre_dSre = grid.dVdS[nodeIdx][leafIdx](0, 0);
                double dVim_dSim = grid.dVdS[nodeIdx][leafIdx](1, 1);
                dVNode_dSLeaves.push_back({dVre_dSre, dVim_dSim});
            }

            result.push_back(std::move(dVNode_dSLeaves));
        }
    }

    return result;
}

std::vector<std::vector<std::array<double, 2>>> PowerFlowSolver::getDiDs() const
{
    if (!settings.compute_gradients)
    {
        throw std::runtime_error("Gradients are unavailable. Enable 'compute_gradients' in settings and ensure the network uses Backward Forward Sweep.");
    }

    std::vector<std::vector<std::array<double, 2>>> result;

    for (Grid const &grid : network->grids)
    {
        for (size_t edgeIdx = 0; edgeIdx < grid.dIdS.size(); ++edgeIdx)
        {
            std::vector<std::array<double, 2>> dIEdge_dSLeaves;

            for (size_t leafIdx = 0; leafIdx < grid.dIdS[edgeIdx].size(); ++leafIdx)
            {
                double dIre_dSre = grid.dIdS[edgeIdx][leafIdx](0, 0);
                double dIim_dSim = grid.dIdS[edgeIdx][leafIdx](1, 1);
                dIEdge_dSLeaves.push_back({dIre_dSre, dIim_dSim});
            }

            result.push_back(std::move(dIEdge_dSLeaves));
        }
    }

    return result;
}

std::vector<std::vector<std::array<double, 2>>> PowerFlowSolver::getDsDs() const
{
    if (!settings.compute_gradients)
    {
        throw std::runtime_error("Gradients are unavailable. Enable 'compute_gradients' in settings and ensure the network uses Backward Forward Sweep.");
    }

    std::vector<std::vector<std::array<double, 2>>> result;

    for (Grid const &grid : network->grids)
    {
        for (size_t nodeIdx = 0; nodeIdx < grid.dSdS.size(); ++nodeIdx)
        {
            // We only want to get dS_all_nodes_except_leaf/dS_all_leaves,
            // so skip LOAD (leaf) nodes.
            if (grid.nodes[nodeIdx].type == NodeType::LOAD)
            {
                continue;
            }

            std::vector<std::array<double, 2>> dSNode_dSLeaves;
            for (size_t leafIdx = 0; leafIdx < grid.dSdS[nodeIdx].size(); ++leafIdx)
            {
                double dSre_dSre = grid.dSdS[nodeIdx][leafIdx](0, 0);
                double dSim_dSim = grid.dSdS[nodeIdx][leafIdx](1, 1);
                dSNode_dSLeaves.push_back({dSre_dSre, dSim_dSim});
            }

            result.push_back(std::move(dSNode_dSLeaves));
        }
    }

    return result;
}

std::vector<std::vector<std::array<double, 2>>> PowerFlowSolver::getDslossDs() const
{
    if (!settings.compute_gradients)
    {
        throw std::runtime_error("Gradients are unavailable. Enable 'compute_gradients' in settings and ensure the network uses Backward Forward Sweep.");
    }

    std::vector<std::vector<std::array<double, 2>>> result;

    for (Grid const &grid : network->grids)
    {
        for (size_t edgeIdx = 0; edgeIdx < grid.dSlossdS.size(); ++edgeIdx)
        {
            std::vector<std::array<double, 2>> dSlossEdge_dSLeaves;

            for (size_t leafIdx = 0; leafIdx < grid.dSlossdS[edgeIdx].size(); ++leafIdx)
            {
                double dSlossre_dSre = grid.dSlossdS[edgeIdx][leafIdx](0, 0);
                double dSlossim_dSim = grid.dSlossdS[edgeIdx][leafIdx](1, 1);
                dSlossEdge_dSLeaves.push_back({dSlossre_dSre, dSlossim_dSim});
            }

            result.push_back(std::move(dSlossEdge_dSLeaves));
        }
    }

    return result;
}

// Resets powers to 0 and voltages to 1
void PowerFlowSolver::reset()
{
    for (std::unique_ptr<GridSolver> &solver : gridSolvers)
    {
        solver->reset();
    }
}

void PowerFlowSolver::save(std::ofstream &file)
{
    saveNetwork(network, file);
}

bool PowerFlowSolver::isRadial()
{
    GridAnalyzer analyzer;
    for (Grid& grid : network->grids)
    {
        if (analyzer.determineSolver(grid) != BACKWARDFOWARDSWEEP)
        {
            return false;
        }
    }

    return true;
}

void PowerFlowSolver::simplifyNetwork()
{
    if (!isRadial())
    {
        throw new std::runtime_error("Network has cycles. Can only simplify radial networks.");
    }

    std::vector<std::vector<node_idx_t>> netNodeMap(network->grids.size());

    for (size_t i = 0; i < network->grids.size(); i++)
    {
        Grid &grid = network->grids[i];
        simplify(grid, 0);

        // Rebuild grid
        Grid newGrid;
        newGrid.sBase = grid.sBase;
        newGrid.vBase = grid.vBase;

        // Remap nodes and edges
        std::vector<node_idx_t> nodeMap(grid.nodes.size(), -1);
        std::vector<edge_idx_t> edgeMap(grid.edges.size(), -1);

        node_idx_t newNodeId = 0;
        for (size_t j = 0; j < grid.nodes.size(); j++)
        {
            if (grid.nodes[j].type != REMOVED)
            {
                nodeMap[j] = newNodeId;
                newNodeId++;
            }
        }

        node_idx_t newEdgeId = 0;
        for (size_t j = 0; j < grid.edges.size(); j++)
        {
            if (grid.edges[j].parent != -1)
            {
                edgeMap[j] = newEdgeId;
                newEdgeId++;
            }
        }

        // Rebuild nodes and edges
        newGrid.nodes.resize(newNodeId);
        newGrid.edges.resize(newEdgeId);

        for (size_t j = 0; j < grid.nodes.size(); j++)
        {
            if (nodeMap[j] == -1) continue;
            GridNode &node = grid.nodes[j];
            GridNode &newNode = newGrid.nodes[nodeMap[j]];

            newNode.type = node.type;
            for (edge_idx_t edge : node.edges)
            {
                edge_idx_t newEdge = edgeMap[edge];
                if (newEdge != -1)
                {
                    newNode.edges.push_back(newEdge);
                }
            }
        }

        for (size_t j = 0; j < grid.edges.size(); j++)
        {
            if (edgeMap[j] == -1) continue;
            GridEdge &edge = grid.edges[j];
            GridEdge &newEdge = newGrid.edges[edgeMap[j]];

            newEdge.z_c = edge.z_c;
            newEdge.parent = nodeMap[edge.parent];
            newEdge.child = nodeMap[edge.child];
        }

        network->grids[i] = newGrid;
        netNodeMap[i] = nodeMap;
    }

    // Remap connections
    for (GridConnection &connection : network->connections)
    {
        connection.loadImplicitNode = netNodeMap[connection.loadImplicitGrid][connection.loadImplicitNode];
        connection.slackImplicitNode = netNodeMap[connection.slackImplicitGrid][connection.slackImplicitNode];
    }

    firstRun = true;
    gridSolvers.clear();
}

void PowerFlowSolver::simplify(Grid &grid, node_idx_t n)
{
    GridNode &node = grid.nodes[n];
    const double mergeThreshold = 1e-6;

    // TODO: test this, unsure if it works correctly
    // Pass 1: remove zero-impedance cables
    for (edge_idx_t edgeId : node.edges)
    {
        GridEdge &edge = grid.edges[edgeId];
        if (edge.child == n) continue; 
        if (std::abs(edge.z_c) >= mergeThreshold) continue;

        node_idx_t childIdx = edge.child;
        GridNode &child = grid.nodes[childIdx];

        // Cannot merge if child is a LOAD or SLACK — would lose boundary node
        if (child.type == LOAD || child.type == LOAD_IMPLICIT ||
            child.type == SLACK || child.type == SLACK_IMPLICIT)
            continue;

        // Re-attach all of child's outgoing edges to current node
        for (edge_idx_t childEdgeId : child.edges)
        {
            GridEdge &childEdge = grid.edges[childEdgeId];
            if (childEdge.child == childIdx) continue;  // skip incoming edge

            // Re-parent this edge to current node
            childEdge.parent = n;
            node.edges.push_back(childEdgeId);
        }

        // Mark zero edge and child node for removal
        edge.parent = -1;
        child.type = REMOVED;

        // Remove the zero edge and child from current node's edge list
        node.edges.erase(
            std::remove(node.edges.begin(), node.edges.end(), edgeId),
            node.edges.end()
        );
    }

    // Check if node is candidate for removal
    if (node.type == MIDDLE && node.edges.size() == 2)
    {
        edge_idx_t parentEdgeId, childEdgeId;
        if (grid.edges[node.edges[0]].child == n)
        {
            parentEdgeId = node.edges[0];
            childEdgeId  = node.edges[1];
        }
        else
        {
            parentEdgeId = node.edges[1];
            childEdgeId  = node.edges[0];
        }

        GridEdge *parentEdge = &grid.edges[parentEdgeId];
        GridEdge *childEdge  = &grid.edges[childEdgeId];

        // Merge impedances and re-attach child edge to grandparent
        childEdge->z_c    += parentEdge->z_c;
        childEdge->parent  = parentEdge->parent;

        // This fixes multi-edge chains not being merged correctly
        GridNode &grandparent = grid.nodes[parentEdge->parent];
        for (edge_idx_t &e : grandparent.edges)
        {
            if (e == parentEdgeId)
            {
                e = childEdgeId;
                break;
            }
        }

        // Mark node and parent edge for removal
        parentEdge->parent = -1;
        node.type = REMOVED;

        simplify(grid, childEdge->child);
        return;
    }

    // Recurse into children
    for (edge_idx_t edgeId : node.edges)
    {
        GridEdge &edge = grid.edges[edgeId];
        if (edge.child != n)
        {
            simplify(grid, edge.child);
        }
    }
}
