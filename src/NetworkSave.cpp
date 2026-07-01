#include "powerflow/NetworkSave.hpp"


void saveNetwork(std::shared_ptr<Network> network, std::ofstream& file)
{
    for (Grid grid : network->grids)
    {
        file << "grid\n" << grid.sBase << " " << grid.vBase << "\n";
        for (GridEdge edge : grid.edges)
        {
            file << edge.parent << " " << edge.child << " (" 
            << edge.z_c.real() << ", " << edge.z_c.imag() << ")\n";
        }

        file << "\%\n";

        for (size_t i = 0; i < grid.nodes.size(); i++)
        {
            GridNode node = grid.nodes[i];
            switch (node.type)
            {
            case SLACK:
                file << i << " s\n";
                break;
            case SLACK_IMPLICIT:
                file << i << " si\n";
                break;
            case LOAD:
                file << i << " l\n";
                break;
            case LOAD_IMPLICIT:
                file << i << " li\n";      
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