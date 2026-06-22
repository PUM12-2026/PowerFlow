// Util to save networks to file

#pragma once

#include <fstream>
#include "network.hpp"

void saveNetwork(std::shared_ptr<Network> network, std::ofstream& file);