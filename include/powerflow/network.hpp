#ifndef POWERFLOW_NETWORK_H
#define POWERFLOW_NETWORK_H

#include <vector>
#include <complex>
#include <Eigen/Core>

// Type definitions for the project
using complex_t = std::complex<double>;
using node_idx_t = int;
using grid_idx_t = int;
using edge_idx_t = int;

static const double SQRT3 = 1.73205080757;

// Graph edge struct.
struct GridEdge
{
    node_idx_t parent = -1;
    node_idx_t child = -1;
    complex_t z_c{1};
};

// Possible node types.
enum NodeType
{
    LOAD,
    LOAD_IMPLICIT,
    MIDDLE,
    SLACK,
    SLACK_IMPLICIT,
    
    // Used for modification of grid
    REMOVED
};

// Graph node struct.
struct GridNode
{
    NodeType type = NodeType::MIDDLE;
    // Complex voltage
    complex_t v = 1;
    // Complex power injection
    complex_t s = 0;
    std::vector<edge_idx_t> edges{};
};

// Edge between two grids.
struct GridConnection
{
    grid_idx_t loadImplicitGrid{};
    grid_idx_t slackImplicitGrid{};
    node_idx_t loadImplicitNode{};
    node_idx_t slackImplicitNode{};
};

struct Grid
{
    std::vector<GridEdge> edges{};
    std::vector<GridNode> nodes{};
    double sBase = 1;
    double vBase = 1;

    // NOTE: Only exists when computing with BFS through the networks,
    // and used to store the gradients of each node in the grid, except
    // households, with respect to the effect of each household node.
    std::vector<std::vector<Eigen::Matrix2d>> dSdS{};

    // NOTE: Only exists when computing with BFS through the networks,
    // and used to store the gradients of each cable in the grid with
    // respect to the effect of each household node.
    std::vector<std::vector<Eigen::Matrix2d>> dSlossdS{};

    // NOTE: Only exists when computing with BFS through the networks,
    // and used to store the gradients of each cable in the grid with
    // respect to the effect of each household node.
    std::vector<std::vector<Eigen::Matrix2d>> dIdS{};

    // NOTE: Only exists when computing with BFS through the networks,
    // and used to store the gradients of each node, except the slack (root) node,
    // in the grid with respect to the effect of each household node.
    std::vector<std::vector<Eigen::Matrix2d>> dVdS{};
};

// Network of grids with connections between them.
struct Network
{
    std::vector<Grid> grids{};
    std::vector<GridConnection> connections{};
};

#endif
