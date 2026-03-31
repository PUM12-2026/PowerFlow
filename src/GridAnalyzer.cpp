#include "powerflow/GridAnalyzer.hpp"

#include <vector>
#include <iostream>
#include <stack>
#include <unordered_set>

static const double ZBUSJACOBI_NODE_LIMIT = 10000;

SolverType GridAnalyzer::determineSolver(Grid const& grid)
{
    if (isSuitableForBFS(grid))
    {
        return BACKWARDFOWARDSWEEP;
    }
    else if (hasSingleSlackNode(grid) && grid.nodes.size() < ZBUSJACOBI_NODE_LIMIT)
    { 
        return ZBUSJACOBI;
    }
    else
    {
        return GAUSSSEIDEL;
    }
}

bool GridAnalyzer::isSuitableForBFS(Grid const& grid)
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

        // Insert as visited, if not successful we have already visited, cycle!
        auto [_, success] = visited_nodes.insert(current_node);

        if (!success)
        {
            return false;
        }

        // add neighbors to stack
        GridNode current_grid_node = grid.nodes[current_node];

        // Verify that LOAD/LOAD_IMPLICIT nodes are leaf nodes.
        if ((current_grid_node.type == NodeType::LOAD ||
            current_grid_node.type == NodeType::LOAD_IMPLICIT) &&
            current_grid_node.edges.size() != 1) 
        {
            return false;
        }

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
    return hasSingleSlackNode(grid);
}

bool GridAnalyzer::hasSingleSlackNode(Grid const& grid)
{   
    //basic for-loop that counts how all slacknodes in a grid. 
    int slackNodes = 0;

    for (const GridNode& node : grid.nodes)
    {
        if (node.type == SLACK_IMPLICIT || node.type == SLACK)
        {
            slackNodes++;
        }
    }
    //Return true if only 1 exists
    return slackNodes == 1; 
}
