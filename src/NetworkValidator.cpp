#include "powerflow/NetworkValidator.hpp"

#include <stack>
#include <set>
#include <unordered_set>

void NetworkValidator::validateNetwork(const Network& network)
{
    validateConnections(network);
    for (grid_idx_t gridIdx = 0; gridIdx < network.grids.size(); ++gridIdx)
    {
        validateGrid(network.grids.at(gridIdx), gridIdx);
    }
}

void NetworkValidator::validateConnections(const Network& network)
{
    std::set<std::tuple<grid_idx_t, node_idx_t>> middleNodes; //Isn't used 
    std::set<std::tuple<grid_idx_t, node_idx_t>> slackNodes; //Isn't used 

    for (size_t connIdx = 0; connIdx < network.connections.size(); ++connIdx) //Loop through all edges in a network
    { 
        const GridConnection& conn = network.connections.at(connIdx);

        if (conn.loadImplicitGrid < 0 || conn.loadImplicitGrid >= network.grids.size()) //Check that the edge is in range of the network. 
        {
            throw std::invalid_argument("Invalid grid number " + 
                std::to_string(conn.loadImplicitGrid) + " in connection " + 
                std::to_string(connIdx));
        }
        if (conn.slackImplicitGrid < 0 || conn.slackImplicitGrid >= network.grids.size()) //index checking
        {
            throw std::invalid_argument("Invalid grid number " +
                std::to_string(conn.slackImplicitGrid) + " in connection " +
                std::to_string(connIdx));
        }
        if (conn.loadImplicitGrid == conn.slackImplicitGrid) //A network shouldn't be connected to itself
        {
            throw std::invalid_argument("Connection in the same grid " + 
                std::to_string(conn.loadImplicitGrid) + " not allowed");
        }
        if (conn.loadImplicitNode < 0 || conn.loadImplicitNode >= network.grids.at(conn.loadImplicitGrid).nodes.size())
        {
            throw std::invalid_argument("Invalid node number " +
                std::to_string(conn.loadImplicitNode) + " for grid " +
                std::to_string(conn.loadImplicitGrid) + " in connection " +
                std::to_string(connIdx));
        }
        if (conn.slackImplicitNode < 0 || conn.slackImplicitNode >= network.grids.at(conn.slackImplicitGrid).nodes.size())
        {
            throw std::invalid_argument("Invalid node number " +
                std::to_string(conn.slackImplicitNode) + " for grid " +
                std::to_string(conn.slackImplicitGrid) + " in connection " +
                std::to_string(connIdx));
        }
        if (network.grids.at(conn.loadImplicitGrid).nodes.at(conn.loadImplicitNode).type != LOAD_IMPLICIT)
        {
            throw std::invalid_argument("Invalid node type in connection " + std::to_string(connIdx));
        }
        if (network.grids.at(conn.slackImplicitGrid).nodes.at(conn.slackImplicitNode).type != SLACK_IMPLICIT)
        {
            throw std::invalid_argument("Invalid node type in connection " + std::to_string(connIdx));
        }
    }

    // Verify that every SLACK_IMPLICIT/LOAD_IMPLICIT node has one and only one
    // associated connection.
    for (grid_idx_t gridIdx = 0; gridIdx < network.grids.size(); ++gridIdx)
    {
        const Grid& grid = network.grids.at(gridIdx);

        for (node_idx_t nodeIdx = 0; nodeIdx < grid.nodes.size(); ++nodeIdx)
        {
            const GridNode& node = grid.nodes.at(nodeIdx);

            if (node.type == SLACK_IMPLICIT || node.type == LOAD_IMPLICIT)
            {
                int conns = 0; // Number of connections to this node

                for (const GridConnection& conn : network.connections)
                {
                    if ((node.type == SLACK_IMPLICIT && conn.slackImplicitGrid == gridIdx &&
                        conn.slackImplicitNode == nodeIdx) ||
                        (node.type == LOAD_IMPLICIT && conn.loadImplicitGrid == gridIdx &&
                            conn.loadImplicitNode == nodeIdx))
                    {
                        ++conns;
                    }
                }
                if (conns != 1)
                {
                    throw std::invalid_argument("Grid " + std::to_string(gridIdx) + 
                        " not properly connected to the rest of the network");
                }
            }
        }
    }
}

void NetworkValidator::validateGrid(const Grid& grid, const grid_idx_t gridIdx)
{
    if (grid.sBase == 0 || grid.vBase == 0)
    {
        throw std::invalid_argument("Invalid base in grid " + std::to_string(gridIdx));
    }

    for (edge_idx_t edgeIdx = 0; edgeIdx < grid.edges.size(); ++edgeIdx)
    {
        const GridEdge& edge = grid.edges.at(edgeIdx);

        if (edge.z_c == 0.0) //Impedance shouldn't be 0 
        {
            throw std::invalid_argument("Invalid zero impedance in edge " +
                std::to_string(edgeIdx) + " in grid " + std::to_string(gridIdx));
        }
        if (edge.parent < 0 || edge.parent >= grid.nodes.size()) //index for parent
        {
            throw std::invalid_argument("Invalid edge " + std::to_string(edgeIdx) + 
                " parent in grid " + std::to_string(gridIdx));
        }
        if (edge.child < 0 || edge.child >= grid.nodes.size()) //index for childnode
        {
            throw std::invalid_argument("Invalid edge " + std::to_string(edgeIdx) +
                " child in grid " + std::to_string(gridIdx));
        }
        if (edge.child == edge.parent) // An edge shouldn't be connected to itself.
        {
            throw std::invalid_argument("Invalid edge " + std::to_string(edgeIdx) +
                " that connects to the same node in grid " + std::to_string(gridIdx));
        }
        for (edge_idx_t edgeIdx2 = 0; edgeIdx2 < grid.edges.size(); ++edgeIdx2)
        {
            if (edgeIdx2 == edgeIdx)
                continue;

            const GridEdge& edge2 = grid.edges.at(edgeIdx2);

            if ((edge2.child == edge.child && edge2.parent == edge.parent) ||
                (edge2.child == edge.parent && edge2.parent == edge.child)) //Check for dubble edges 
            {
                throw std::invalid_argument("More than one edge detected between node " +
                    std::to_string(edge.parent) + " and node " + std::to_string(edge.child) +
                    " in grid " + std::to_string(gridIdx));
            }
        }
    }
    //Check that a slack node exist
    bool foundSlackNode = false;
    for (node_idx_t nodeIdx = 0; nodeIdx < grid.nodes.size(); ++nodeIdx)
    {
        const GridNode& node = grid.nodes.at(nodeIdx);

        if (node.type == SLACK || node.type == SLACK_IMPLICIT)
        {
            foundSlackNode = true;
            break;
        }
    }

    if (!foundSlackNode)
    {
        throw std::invalid_argument("Missing slack node in grid " + std::to_string(gridIdx));
    }

    if (gridIsDisjoint(grid))
    {
        throw std::invalid_argument("Grid " + std::to_string(gridIdx) + " consists of multiple disjoint graphs");
    }
}

bool NetworkValidator::gridIsDisjoint(Grid const& grid)
 //Checking for nodes that are not connected to the grid using DFS
{
    std::stack<node_idx_t> todo_list{};
    std::unordered_set<node_idx_t> visited_nodes{};
    std::unordered_set<edge_idx_t> visited_edges{};

    // Push root node
    todo_list.push(0);

    while (!todo_list.empty())
    {
        // Get current node
        node_idx_t current_node = todo_list.top();
        todo_list.pop();
        // Insert as visited,
        visited_nodes.insert(current_node);

        // add neighbors to stack
        GridNode current_grid_node = grid.nodes[current_node];

        // for all edges of current node, add neighbours to stack
        for (edge_idx_t id : current_grid_node.edges)
        {

            // If edge exists in visited, we shall not go there again
            if (visited_edges.find(id) != visited_edges.end())
            {
                continue;
            }

            node_idx_t parent{ grid.edges[id].parent };
            node_idx_t child{ grid.edges[id].child };

            // If node is child --> push parent to stack
            if (parent != current_node)
            {
                todo_list.push(parent);
            } // If node is parent --> push child to stack
            else if (child != current_node)
            {
                todo_list.push(child);
            }
            else
            {
                throw std::runtime_error("Error! found edge with myself not in it!");
            }
            visited_edges.insert(id);
        }
    }
    return !(visited_nodes.size() == grid.nodes.size());
}
