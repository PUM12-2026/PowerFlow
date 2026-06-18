#include <algorithm>
#include <unordered_set>

#include "powerflow/ParameterValidator.hpp"

ParameterValidator::ParameterValidator(Grid* grid, Logger* const logger, const std::unordered_map<node_idx_t, complex_t> &measuredV, double precision)
    : grid(grid), logger(logger), measuredV(measuredV), precision(precision) {}


void ParameterValidator::validate()
{
    for (auto &[key, val]: measuredV)
    {
        if (std::abs(val) == 0)
        {
            std::cerr << "Measured voltage at node " << key << " is zero. Voltages cannot be zero." << std::endl;
            return;
        }
    }
    validateRecursive(0);
}

std::tuple<complex_t, complex_t, complex_t, bool> ParameterValidator::validateRecursive(node_idx_t nodeId)
{
    /*
    In node n:
        if leaf node:
            compute I_parent = (S_n/U_rn)* where U_rn is measured (true) voltage of node n
            return I_parent, S_n, U_rn
        else:
            for each child:
                call function recursively to get I_child := I_parent, S_child := S_n, U_child := U_n
                compute dU = I_child * Z where dU is change in voltage between child and parent
                compute U = U_child + dU
                compute S_loss = dU * I
            compute S = sum(S_child) + sum(S_loss)
            compare all calculated U, if mismatch => error, return null values

            compute I_parent = (S / U_n)* - sum(I_child) using KCL

            return I_parent, S, U_n
    */

    if (nodeId == -1) return{{0, 0}, {0, 0}, {0, 0}, false};    

    bool isLeaf = (grid->nodes[nodeId].edges.size() == 1 && nodeId != 0);

    if (isLeaf)
    {
        if (measuredV.find(nodeId) == measuredV.end())
        {
            return{{0, 0}, {0, 0}, {0, 0}, false};   
        }

        complex_t S = grid->nodes[nodeId].s;
        // Assume voltage phase angle is equal to slack node phase angle
        //complex_t U = std::polar(std::abs(measuredV.at(nodeId)), std::arg(grid->nodes[0].v));
        complex_t U = measuredV.at(nodeId);
        complex_t I = std::conj(S / U);

        return {I, S, U, true};
    }

    complex_t I = {0, 0};
    complex_t S = {0, 0};

    // What each child node thinks the parent's voltage should be
    std::vector<std::tuple<complex_t, node_idx_t>> voltages;
    // Current from each child to parent
    std::unordered_map<node_idx_t, complex_t> currents;
    // Each child's voltage
    std::unordered_map<node_idx_t, complex_t> childVoltages;

    // Loop through all adjacent nodes
    for (edge_idx_t edgeId : grid->nodes[nodeId].edges)
    {
        node_idx_t childId = grid->edges[edgeId].child;
        // Ignore parent edge, otherwise we get infinite recursion loops
        if (childId == nodeId) continue;
            
        std::tuple<complex_t, complex_t, complex_t, bool> childISU = validateRecursive(childId);
        if (!std::get<bool>(childISU)) 
        {
            // Invalid child
            continue;
        }
        
        complex_t childI = std::get<0>(childISU);
        complex_t childS = std::get<1>(childISU);
        complex_t childU = std::get<2>(childISU);

        currents[childId] = childI;
        childVoltages[childId] = childU;
        
        complex_t deltaU = childI * grid->edges[edgeId].z_c;
        complex_t U = childU + deltaU;
        voltages.push_back({U, childId});

        // Loss of S in cable
        complex_t SLoss = deltaU * std::conj(childI);

        S += childS + SLoss;
        I += childI;
    }

    // If majority of children are invalid, parent is also invalid
    if (voltages.size() * 2 <= (grid->nodes[nodeId].edges.size() - 1))
    {
        return {{0, 0}, {0, 0}, {0, 0}, false};
    }

    // Each cluster is a set of child nodes who agree (within precision) about what the parent's voltage should be
    std::vector<std::vector<std::tuple<complex_t, node_idx_t>>> clusters;

    // Loop over all voltages twice. If two voltages within precision => add to cluster
    for (size_t i = 0; i < voltages.size(); i++)
    {
        clusters.push_back({voltages[i]});

        for (size_t j = 0; j < voltages.size(); j++)
        {
            if (i == j) continue;
            
            complex_t U1 = std::get<complex_t>(voltages[i]);
            complex_t U2 = std::get<complex_t>(voltages[j]);

            if (std::abs(U1 - U2) <= precision)
            {
                clusters[i].push_back({U2, std::get<node_idx_t>(voltages[j])});
            }
        }
    }

    for (auto cluster : clusters)
    {
        // Check if any cluster is majority
        if (cluster.size() * 2 > voltages.size())
        {
            // We have found a cluster which is a majority
            // Compute U at current node, and I going to parent of current node

            // Some variation in voltage may occur, therefore we compute the average instead of just picking one
            complex_t averageU = {0, 0};
            for (size_t i = 0; i < cluster.size(); i++)
            {
                complex_t U = std::get<complex_t>(cluster[i]);
                averageU += U;
            }
            averageU /= static_cast<double>(cluster.size());
            
            complex_t parentI = std::conj(S / averageU);

            // Find any nodes not in majority cluster
            for (auto node1 : voltages)
            {
                bool isInValidCluster = false;
                for (auto node2 : cluster)
                {
                    if (std::get<node_idx_t>(node1) == std::get<node_idx_t>(node2)) 
                    {
                        isInValidCluster = true;
                        break;
                    }
                }

                if (!isInValidCluster)
                {
                    node_idx_t childId = std::get<node_idx_t>(node1);
                    complex_t nominalZ;
                    if (std::abs(currents[childId]) == 0)
                    {
                        continue;
                    }
                    complex_t suggestedZ = (averageU - childVoltages[childId]) /  currents[childId];

                    for (edge_idx_t edgeId : grid->nodes[childId].edges)
                    {
                        if (grid->edges[edgeId].parent == nodeId)
                        {
                            nominalZ = grid->edges[edgeId].z_c;
                            // Update impedance
                            grid->edges[edgeId].z_c = suggestedZ;
                        }
                    }
                    
                    *logger << "Cable parameter error likely between node " << nodeId << " and " << childId
                    << ". Impedance is: " << nominalZ << ", Impedance should be: " << suggestedZ << std::endl;
                }
            }

            return {parentI, S, averageU, true};
        }
    }

    *logger << "Cable parameter error identified at unknown child/children of node " << nodeId << std::endl;

    return{{0, 0}, {0, 0}, {0, 0}, false};    
}


// Compute branch currents in network at time step t, recursively. 
// Ignores losses of power in cables, as per original algorithm
complex_t ParameterValidator::BackwardSweep(node_idx_t n, size_t t, std::vector<complex_t> &branchCurrents)
{
    if (grid->nodes[n].type == LOAD || grid->nodes[n].type == LOAD_IMPLICIT)
    {
        complex_t U = measuredValues[n].U[t];
        complex_t S = measuredValues[n].S[t];
        complex_t I = std::conj(S / U);
        
        return I;
    }

    complex_t I = {0, 0};
    for (edge_idx_t edge : grid->nodes[n].edges)
    {
        node_idx_t child = grid->edges[edge].child;
        if (child == n) continue;

        // Current from this node to child node
        complex_t childI = BackwardSweep(child, t, branchCurrents);
        branchCurrents[edge] = childI;

        I += childI;
    }

    return I;
}


void ParameterValidator::GetDownStream(node_idx_t n, std::vector<edge_idx_t> &edges, std::vector<node_idx_t> &loads)
{
    if (grid->nodes[n].type == LOAD || grid->nodes[n].type == LOAD_IMPLICIT)
    {
        loads.push_back(n);
    }

    for (edge_idx_t edge : grid->nodes[n].edges)
    {
        node_idx_t child = grid->edges[edge].child;
        if (child == n) continue;
        
        edges.push_back(edge);
        GetDownStream(child, edges, loads);
    }
}


bool ParameterValidator::FindPath(node_idx_t n, node_idx_t m, std::vector<edge_idx_t> &path)
{
    node_idx_t current = m;
    while (current != n)
    {
        for (edge_idx_t edge : grid->nodes[current].edges)
        {
            node_idx_t child = grid->edges[edge].child;
            if (child != current) continue;

            current = grid->edges[edge].parent;
            path.push_back(edge);
        }

        // Reached top of tree without finding path
        if (current != n && grid->nodes[current].type == SLACK)
        {
            return false;
        }
    }

    return true;
}


void ParameterValidator::EstimateParameters(std::vector<std::vector<complex_t>> &branchCurrents,
    std::vector<complex_t> &slackVoltages, std::vector<complex_t> &newImpedances)
{
    const size_t timeSteps = branchCurrents.size();

    for (edge_idx_t edge : grid->nodes[0].edges)
    {
        node_idx_t child = grid->edges[edge].child;
        if (child == 0) continue;

        // Slack parent and load child => can do regression directly, 
        // eq 17 from https://ietresearch.onlinelibrary.wiley.com/doi/10.1049/stg2.12177
        // NOTE net_bad_param.txt will not test this, as slack has no LOAD children
        if (grid->nodes[child].type == LOAD || grid->nodes[child].type == LOAD_IMPLICIT)
        {
            Eigen::MatrixXd A(timeSteps, 2);
            Eigen::VectorXd b(timeSteps);

            for (size_t t = 0; t < timeSteps; t++)
            {
                complex_t u = measuredValues[child].U[t];
                double angle = std::arg(u);

                complex_t j = std::conj(measuredValues[child].S[t] / u);
                complex_t j_rot = j * std::exp(complex_t(0, -angle));

                A(t, 0) = j_rot.real();
                A(t, 1) = -j_rot.imag();
                b(t) = std::abs(slackVoltages[t]) - std::abs(u);
            }

            Eigen::VectorXd Z = (A.transpose() * A).ldlt().solve(A.transpose() * b);
            newImpedances[edge] = {Z(0), Z(1)};
        }

        // Slack parent and middle child => do regression while including all downstream nodes,
        // use eq 21 from https://ietresearch.onlinelibrary.wiley.com/doi/10.1049/stg2.12177
        else if (grid->nodes[child].type == MIDDLE)
        {
            std::vector<edge_idx_t> edges;
            std::vector<node_idx_t> loads;
            edges.push_back(edge);
            GetDownStream(child, edges, loads);

            Eigen::MatrixXd A(loads.size() * timeSteps, 2 * edges.size());
            Eigen::VectorXd b(loads.size() * timeSteps);

            int row = 0;
            for (size_t d = 0; d < loads.size(); d++)
            {
                node_idx_t load = loads[d];
                std::vector<edge_idx_t> path;
                FindPath(child, load, path);
                path.push_back(edge);

                std::unordered_set<edge_idx_t> pathSet(path.begin(), path.end());

                for (size_t t = 0; t < timeSteps; t++)
                {
                    complex_t u = measuredValues[load].U[t];
                    double angle = std::arg(u);

                    for (size_t k = 0; k < edges.size(); k++)
                    {    

                        // Check if edge is on path to load
                        if (pathSet.count(edges[k]))
                        {
                            complex_t j = branchCurrents[t][edges[k]];
                            complex_t j_rot = j * std::exp(complex_t(0, -angle));

                            A(row, 2 * k) = j_rot.real();
                            A(row, 2 * k + 1) = -j_rot.imag();
                        }
                        else
                        {
                            A(row, 2 * k) = 0;
                            A(row, 2 * k + 1) = 0;
                        }
                    }

                    b(row) = std::abs(slackVoltages[t]) - std::abs(u);
                    row++;
                }
            }

            Eigen::VectorXd Z = (A.transpose() * A).ldlt().solve(A.transpose() * b);
            for (auto i = 0; i * 2 < Z.rows(); i++)
            {
                newImpedances[edges[i]] = {Z(i * 2), Z(i * 2 + 1)};
            }
        }
    }
}


void ParameterValidator::ForwardSweep(node_idx_t n, size_t t, std::vector<complex_t> &branchCurrents, 
    complex_t parentVoltage, edge_idx_t parentEdge)
{
    if (grid->nodes[n].type == SLACK || grid->nodes[n].type == SLACK_IMPLICIT)
    {
        for (edge_idx_t edge : grid->nodes[n].edges)
        {
            ForwardSweep(grid->edges[edge].child, t, branchCurrents, parentVoltage, edge);
        }
        return;
    }

    complex_t estimatedVoltage = parentVoltage - grid->edges[parentEdge].z_c * branchCurrents[parentEdge];

    if (grid->nodes[n].type == LOAD || grid->nodes[n].type == LOAD_IMPLICIT)
    {
        // Equation 19 https://ietresearch.onlinelibrary.wiley.com/doi/10.1049/stg2.12177
        double magnitude = std::abs(measuredValues[n].U[t]);
        double angle = std::atan2(estimatedVoltage.imag(), estimatedVoltage.real());
        measuredValues[n].U[t] = std::polar(magnitude, angle);
    }

    for (edge_idx_t edge : grid->nodes[n].edges)
    {
        node_idx_t child = grid->edges[edge].child;
        if (child == n) continue;
        ForwardSweep(grid->edges[edge].child, t, branchCurrents, estimatedVoltage, edge);
    }
}


std::vector<complex_t> ParameterValidator::validateRegression(std::unordered_map<node_idx_t, MeasuredValues> &measuredValues, 
    std::vector<complex_t> &slackVoltages, double convergenceThreshold, int maxIterations)
{
    this->measuredValues = measuredValues;
    
    const size_t edgeCount = grid->edges.size();
    const size_t timeSteps = slackVoltages.size();

    // Validate amount of samples (time steps)
    for (auto &[key, val] : measuredValues)
    {
        if (val.S.size() != timeSteps)
        {
            std::cerr << "Node " << key << " has invalid amount of samples. Expected " << timeSteps << ", got " << val.S.size() << std::endl;
            return {};
        }
        if (val.U.size() != timeSteps)
        {
            std::cerr << "Node " << key << " has invalid amount of samples. Expected " << timeSteps << ", got " << val.U.size() << std::endl;
            return {};
        }
    }

    // Validate voltages
    for (auto &[key, val] : measuredValues)
    {
        for (complex_t u : val.U)
        {
            if (std::abs(u) == 0)
            {
                std::cerr << "Voltage at node " << key << " is zero. Voltages may not be zero." << std::endl;
                return {};
            }
        }
    }

    // Initialise voltages for each time t
    for (size_t t = 0; t < timeSteps; t++)
    {
        // Slack voltage phase angle
        double angle = std::arg(slackVoltages[t]);

        for (auto& it : measuredValues)
        {
            // Assume phase angle of load voltages = phase angle of slack voltage
            it.second.U[t] = std::polar(std::abs(it.second.U[t]), angle);
        }
    }

    // Cache impedances in case we don't converge
    std::vector<complex_t> oldImpedances(edgeCount);
    for (size_t i = 0; i < edgeCount; i++)
    {
        oldImpedances[i] = grid->edges[i].z_c;
    }

    std::vector<complex_t> newImpedances(edgeCount);
    for (int i = 0; i < maxIterations; i++)
    {
        // Backward sweep, compute branch currents
        std::vector<std::vector<complex_t>> branchCurrents(timeSteps, std::vector<complex_t>(edgeCount));
        for (size_t t = 0; t < timeSteps; t++)
        {
            BackwardSweep(0, t, branchCurrents[t]);
        }

        // Estimate impedances from branch currents using ordinary least squares (OLS) regression
        EstimateParameters(branchCurrents, slackVoltages, newImpedances);

        // Check convergence
        const float epsilon = 1e-10;
        bool converged = true;

        for (auto j = 0; j < grid->edges.size(); j++)
        {
            GridEdge edge = grid->edges[j];
            complex_t oldZ = edge.z_c;
            complex_t newZ = newImpedances[j];

            if (std::abs(oldZ - newZ) >= convergenceThreshold)
            {
                converged = false;
            }

            if (std::abs(newZ) > epsilon)
            {
                grid->edges[j].z_c = newZ;
            }
        }
        if (converged)
        {
            return newImpedances;
        }

        // Forward sweep, adjust voltage angles at load nodes
        for (size_t t = 0; t < timeSteps; t++)
        {
            ForwardSweep(0, t, branchCurrents[t], slackVoltages[t], -1);
        }
    }

    std::cout << "Failed to converge (max iterations = " << maxIterations << ")\n";

    // Restore parameters
    for (size_t i = 0; i < edgeCount; i++)
    {
        grid->edges[i].z_c = oldImpedances[i];
    }

    return newImpedances;
}
