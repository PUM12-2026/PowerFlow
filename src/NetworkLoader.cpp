#include "powerflow/NetworkLoader.hpp"
#include "powerflow/NetworkLoaderError.hpp"
#include <set>
#include <iostream>

/// Loads a network from a network file. Read README.md for format and check the examples for reference.
NetworkLoader::NetworkLoader(std::istream& file) : file{ file } { }

std::unique_ptr<Network> NetworkLoader::loadNetwork()
{
    std::unique_ptr<Network> network = std::make_unique<Network>();
    std::string line;

    while (getNextLine(line))
    {
        // Checks how the program should interepret the line.
        if (line == "grid")
        {
            network->grids.push_back(loadGrid());
        }
        else if (line == "connections")
        {
            network->connections = loadConnections();
        }
        else
        {
            throw NetworkLoaderError("Invalid command", curLine);
        }
    }
    return network;
}


    /// Load a grid from the current stream position. Called from the constructor when the line "grid" is read
    /// Expected section layout:
    /// 1. base values line: <S_base> <V_base>
    /// 2. edge list, terminated by '%'
    /// 3. explicit node type list, terminated by '%'

Grid NetworkLoader::loadGrid()
{
    Grid grid;

    getGridBase(grid);

    std::string line;
    std::stringstream sstream{};
    node_idx_t nodeCount = 0; // Number of nodes in the grid
    
    // Get edges.
    while (getNextLine(line))
    {
        // Loops through the lines until it finds a line with only a '%' character which indicates the end of the edges list
        if (line == "%")
            // End of edges list
            break;

        sstream << line;
        node_key_t parentId, childId;
        complex_t z;

        // Catches cases where the line contains invalid values
        if (!(sstream >> parentId) || parentId < 0)
        {
            throw NetworkLoaderError("Invalid edge parent index", curLine);
        }
        if (!(sstream >> childId) || childId < 0)
        {
            throw NetworkLoaderError("Invalid edge child index", curLine);
        }
        if (!(sstream >> z))
        {
            throw NetworkLoaderError("Invalid edge impedance", curLine);
        }

        if (grid.nodeMap.find(parentId) == grid.nodeMap.end())
        {
            node_idx_t parentIdx = nodeCount;
            grid.idMap.push_back(parentId);
            grid.nodeMap[parentId] = parentIdx;

            nodeCount++;
        }
        if (grid.nodeMap.find(childId) == grid.nodeMap.end())
        {
            node_idx_t childIdx = nodeCount;
            grid.idMap.push_back(childId);
            grid.nodeMap[childId] = childIdx;

            nodeCount++;
        }

        edge_idx_t edgeIdx = static_cast<edge_idx_t>(grid.edges.size());
        GridEdge edge;
        edge.parent = grid.nodeMap[parentId];
        edge.child = grid.nodeMap[childId];
        edge.z_c = z;

        // Commented line is for calculating impedence per edge
        // edge.z_c = edge.z_c / ((grid.vBase * grid.vBase) / grid.sBase); // Convert to per-unit

        // If the edge is valid, add it to the grid and update the node count
        grid.edges.push_back(edge);

        // We want to resize the nodeCount size later so that all nodes, and children are included. (+1 since the node indices are 0-based)
        //nodeCount = std::max(nodeCount, std::max(edge.parent + 1, edge.child + 1));

        // Clear the stringstream for the next line
        sstream.str("");
        sstream.clear();
    }

    // Clear the stringstream
    sstream.str("");
    sstream.clear();

    if (nodeCount == 0)
    {
        // If the loaded grid is empty, throw an error
        throw NetworkLoaderError("Empty grid", curLine);
    }


    // Resize to the value calculated by nodeCount
    grid.nodes.resize(nodeCount);

    for (node_idx_t edgeIdx = 0; edgeIdx < grid.edges.size(); ++edgeIdx)
    {
        // For each edge, add the edge index to the edges vector of the parent and child nodes of the edge
        GridEdge& edge = grid.edges[edgeIdx];

        grid.nodes.at(edge.parent).edges.push_back(edgeIdx);
        grid.nodes.at(edge.child).edges.push_back(edgeIdx);
    }

    // Get load/slack nodes
    while (getNextLine(line))
    {
        if (line == "%") // End of node list
            break;

        sstream << line;
        node_key_t nodeId = 0;
        node_idx_t nodeIdx;
        std::string type;

        if (!(sstream >> nodeId) || grid.nodeMap.find(nodeId) == grid.nodeMap.end())
        {
            throw NetworkLoaderError("Invalid node index", curLine);
        }
        if (!(sstream >> type))
        {
            throw NetworkLoaderError("Missing or invalid node type", curLine);
        }

        nodeIdx = grid.nodeMap[nodeId];
        if (static_cast<typename std::vector<GridNode>::size_type>(nodeIdx) >= grid.nodes.size())
        {
            throw NetworkLoaderError("Invalid node index", curLine);
        }

        if (type == "si")
        {
            // Sets the type of the node to SLACK_IMPLICIT if the type is "si"
            grid.nodes.at(nodeIdx).type = NodeType::SLACK_IMPLICIT;
        }
        else if (type == "l")
        {
            // Sets the type of the node to LOAD if the type is "l"
            grid.nodes.at(nodeIdx).type = NodeType::LOAD;
        }
        else if (type == "s")
        {
            // Sets the type of the node to SLACK if the type is "s"
            grid.nodes.at(nodeIdx).type = NodeType::SLACK;
        }
        else if (type == "li")
        {
            // Sets the type of the node to LOAD_IMPLICIT if the type is "li"
            grid.nodes.at(nodeIdx).type = NodeType::LOAD_IMPLICIT;
        }
        else
        {
            throw NetworkLoaderError("Invalid node type", curLine);
        }
        // NOTE: If the node does specify a type, it will be set to MIDDLE

        // Clear the stringstream for the next line
        sstream.str("");
        sstream.clear();
    }
    // The grid is now fully loaded, return it
    return grid;
}


/// Reads the base values for a grid from the next line of the input file.
///
/// The expected format is: <S_base> <V_base>
/// S_base is base power of the grid, and V_base is the base voltage of the grid
void NetworkLoader::getGridBase(Grid& grid)
{
    std::string line;
    std::stringstream sstream{};

    getNextLine(line);
    sstream << line;


    // Catches cases where the line contains invalid values or is missing values
    if (!(sstream >> grid.sBase))
    {
        throw NetworkLoaderError("Invalid S base", curLine);
    }
    if (!(sstream >> grid.vBase))
    {
        throw NetworkLoaderError("Invalid V base", curLine);
    }


    std::string rest;
    std::getline(sstream, rest);


    if (rest.find_first_not_of(" ") != std::string::npos)
    {
        throw NetworkLoaderError("Invalid base line", curLine);
    }
}


/// Loads the list of connections from the network file. Expected format for each line is:
/// <load implicit grid index> <load implicit node index> <slack implicit grid index> <slack implicit node index>
std::vector<GridConnection> NetworkLoader::loadConnections()
{
    std::vector<GridConnection> connections;
    std::string line;
    std::stringstream sstream{};

    while (getNextLine(line))
    {
        if (line == "%") // End of connections list
            break;

        GridConnection connection;
        sstream << line;

        if (!(sstream >> connection.loadImplicitGrid))
        {
            throw NetworkLoaderError("Invalid grid index", curLine);
        }
        if (!(sstream >> connection.loadImplicitNode))
        {
            throw NetworkLoaderError("Invalid LOAD_IMPLICIT node index", curLine);
        }
        if (!(sstream >> connection.slackImplicitGrid))
        {
            throw NetworkLoaderError("Invalid grid index", curLine);
        }
        if (!(sstream >> connection.slackImplicitNode))
        {
            throw NetworkLoaderError("Invalid SLACK_IMPLICIT node index", curLine);
        }

        connections.push_back(connection);

        // Clear the stringstream for the next line
        sstream.str("");
        sstream.clear();
    }
    return connections;
}


/// Returns the next line in the file. Skips empty lines and comment lines (lines starting with '#').
/// True if a line was successfully read, false otherwise.
bool NetworkLoader::getNextLine(std::string& line)
{
    while (std::getline(file, line))
    {
        ++curLine;
        if (!line.empty() && line.at(0) != '#')
        {
            return true;
        }
    }
    return false;
}
