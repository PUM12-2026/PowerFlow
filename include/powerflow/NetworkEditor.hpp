// Util to save networks to file

#pragma once

#include <fstream>
#include <memory>
#include "network.hpp"

class NetworkEditor
{
public:
    void simplify(std::shared_ptr<Network> network);

    void saveNetwork(std::shared_ptr<Network> network, std::ofstream& file);

private:
    void _simplify(Grid &grid, node_idx_t n);
};