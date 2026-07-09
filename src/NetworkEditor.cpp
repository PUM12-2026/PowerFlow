#include "powerflow/NetworkEditor.hpp"


void NetworkEditor::saveNetwork(std::shared_ptr<Network> network, std::ofstream& file)
{
    for (Grid grid : network->grids)
    {
        file << "grid\n" << grid.sBase << " " << grid.vBase << "\n";
        for (GridEdge edge : grid.edges)
        {
            file << grid.idMap[edge.parent] << " " << grid.idMap[edge.child] << " (" 
            << edge.z_c.real() << ", " << edge.z_c.imag() << ")\n";
        }

        file << "\%\n";

        for (size_t i = 0; i < grid.nodes.size(); i++)
        {
            GridNode node = grid.nodes[i];
            switch (node.type)
            {
            case SLACK:
                file << grid.idMap[i] << " s\n";
                break;
            case SLACK_IMPLICIT:
                file << grid.idMap[i] << " si\n";
                break;
            case LOAD:
                file << grid.idMap[i] << " l\n";
                break;
            case LOAD_IMPLICIT:
                file << grid.idMap[i] << " li\n";      
                break;      
            default:
                break;
            }
        }
        file << "\%\n\n";
    }

    file << "connections\n";
    for (GridConnection connection : network->connections)
    {
        file << connection.loadImplicitGrid 
        << " " << connection.loadImplicitNode
        << " " << connection.slackImplicitGrid
        << " " << connection.slackImplicitNode
        << "\n";
    }
    file << "\%\n\n";
}

void NetworkEditor::simplify(std::shared_ptr<Network> network)
{
    std::vector<std::vector<node_idx_t>> netNodeMap(network->grids.size());
    for (size_t i = 0; i < network->grids.size(); i++)
    {
        Grid &grid = network->grids[i];
        _simplify(grid, 0);

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
        newGrid.idMap.resize(newNodeId);

        for (size_t j = 0; j < grid.nodes.size(); j++)
        {
            if (nodeMap[j] == -1) continue;
            GridNode &node = grid.nodes[j];
            GridNode &newNode = newGrid.nodes[nodeMap[j]];

            node_key_t externalId = grid.idMap[j];
            newGrid.nodeMap[externalId] = nodeMap[j];
            newGrid.idMap[nodeMap[j]] = externalId;

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
}


void NetworkEditor::_simplify(Grid &grid, node_idx_t n)
{
    GridNode &node = grid.nodes[n];
    const double mergeThreshold = 1e-6;

    // TODO: test this, unsure if it works correctly
    // Pass 1: remove zero-impedance cables
    std::vector<edge_idx_t> edgesCopy = node.edges;  // copy before iterating
    for (edge_idx_t edgeId : edgesCopy)
    {
        GridEdge &edge = grid.edges[edgeId];
        if (edge.child == n) continue;
        if (std::abs(edge.z_c) >= mergeThreshold) continue;

        node_idx_t childIdx = edge.child;
        GridNode &child = grid.nodes[childIdx];

        if (child.type == LOAD || child.type == LOAD_IMPLICIT ||
            child.type == SLACK || child.type == SLACK_IMPLICIT)
            continue;

        for (edge_idx_t childEdgeId : child.edges)
        {
            GridEdge &childEdge = grid.edges[childEdgeId];
            if (childEdge.child == childIdx) continue;
            childEdge.parent = n;
            node.edges.push_back(childEdgeId);  
        }

        edge.parent = -1;
        child.type = REMOVED;

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

        _simplify(grid, childEdge->child);
        return;
    }

    // Recurse into children
    for (edge_idx_t edgeId : node.edges)
    {
        GridEdge &edge = grid.edges[edgeId];
        if (edge.child != n)
        {
            _simplify(grid, edge.child);
        }
    }
}