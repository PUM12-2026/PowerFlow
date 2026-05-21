#include "powerflow/solvers/ZBusJacobiSolver.hpp"
#include "powerflow/network.hpp"

#include <Eigen/Core>
#include <Eigen/LU>

// Constructor that builds the Z bus/impedance matrix
ZBusJacobiSolver::ZBusJacobiSolver(Grid* grid, Logger* const logger, int maxIter, double precision) 
	: GridSolver(grid, logger, maxIter, precision)
{
	// Check that impedances are not zero
	for (GridEdge& edge : grid->edges)
	{
		if (edge.z_c == 0.0)
		{
			throw std::runtime_error("Invalid 0 impedance detected in ZBusJacobiSolver");
		}
	}

	node_idx_t N = grid->nodes.size();
	V = Eigen::VectorXcd::Zero(N); // Node voltages
	S = Eigen::VectorXcd::Zero(N); // Node powers
	I = Eigen::VectorXcd::Zero(N); // Node currands

	// Y bus matrix
	Eigen::MatrixXcd ybus = Eigen::MatrixXcd::Zero(N, N);
	slackNodeIdx = -1;

	// Construct the Y bus
	for (node_idx_t nodeIdx = 0; nodeIdx < grid->nodes.size(); ++nodeIdx) // "For each row in admittance matrix"
	{
		GridNode& node = grid->nodes.at(nodeIdx);
		
		// Enforce fixed voltage
		if (node.type == NodeType::SLACK_IMPLICIT || node.type == NodeType::SLACK)
		{
			if (slackNodeIdx != -1)
			{
				throw std::runtime_error("Multiple slack nodes detected in grid passed to ZBusJacobiSolver");
			}
			ybus(nodeIdx, nodeIdx) = 1; //Set diagonal to 1
			slackNodeIdx = nodeIdx; // Store slack node index for easy access later
		}
		else 
		{
			// For non slack node, fill Y bus based on connected edges
			for (edge_idx_t edgeIdx : grid->nodes.at(nodeIdx).edges) // "For each column in admittance matrix"
			{
				GridEdge& edge = grid->edges.at(edgeIdx);

				// Find neigbouring node index
				node_idx_t neighborIdx = (edge.child == nodeIdx) ? edge.parent : edge.child;
				complex_t y = (complex_t)1 / edge.z_c; // Calculate admittance
				ybus(nodeIdx, neighborIdx) = y;
				ybus(nodeIdx, nodeIdx) -= y;
			}
		}
	}

	// Check exactly one slack node exitsts
	if (slackNodeIdx == -1)
	{
		throw std::runtime_error("ZBusJacobiSolver: Could not find SLACK_IMPLICIT/SLACK node");
	}

	Z = ybus.inverse(); // Calculate Z bus as inverse of Y bus

	// Not used (debugging)
	int zeros = 0;
	for (int row = 0; row < N; ++row)
	{
		for (int col = 0; col < N; ++col)
		{
			if (Z(row, col) == 0.0)
			{
				zeros++;
			}
		}
	}
}


// Iterative solve for Z bus jacobi
int ZBusJacobiSolver::solve()
{
	node_idx_t N = grid->nodes.size();

	// Transfer node voltages and powers to V and S vectors.
	for (node_idx_t nodeIdx = 0; nodeIdx < N; ++nodeIdx)
	{
		if (grid->nodes[nodeIdx].type == SLACK_IMPLICIT || grid->nodes[nodeIdx].type == SLACK)
		{
			// Slack node power is set to V*conj(V) in ZBus Jacobi.
			S(nodeIdx) = grid->nodes[nodeIdx].v * std::conj(grid->nodes[nodeIdx].v);
		}
		else
		{
			S(nodeIdx) = grid->nodes[nodeIdx].s;
		}
		V(nodeIdx) = grid->nodes[nodeIdx].v;
	}

	int iter = 0;
	bool converged = false;

	while (iter < maxIterations)
	{
		double diff = (V.cwiseProduct(I.conjugate()) - S).cwiseAbs().maxCoeff();

		//Check if solution has converged
		if (diff < precision)
		{
			if (!firstRun)
			{
				converged = true;
				break;
			}
			firstRun = false;
		}

		I = S.cwiseQuotient(V).conjugate();
		V = Z * I;
		iter++;
	}

	// Store calculated node voltages.
	if (iter != 0)
	{
		for (node_idx_t nodeIdx = 0; nodeIdx < N; ++nodeIdx)
		{
			if (nodeIdx != slackNodeIdx)
			{
				grid->nodes[nodeIdx].v = V(nodeIdx);
			}
		}
	}

	if (!converged)
	{
		throw std::runtime_error("ZBusJacobiSolver: The solution did not converge. Maximum number of iterations reached.");
	}

	// Update the slack node power based on solved currents
	if (iter != 0)
	{
		updateSlackPower();
	}
	return iter;
}


// Reset solver for reuse
void ZBusJacobiSolver::reset()
{
	GridSolver::reset();
	node_idx_t N = grid->nodes.size();
	V = Eigen::VectorXcd::Zero(N);
	S = Eigen::VectorXcd::Zero(N);
	I = Eigen::VectorXcd::Zero(N);
	firstRun = true;
}


// Computes updated slack node power based on solved voltages
void ZBusJacobiSolver::updateSlackPower()
{
	complex_t yv = 0;
	complex_t ySum = 0; // Sum of addmittances
	GridNode& slackNode = grid->nodes[slackNodeIdx];

	// Loop over edges connected to slack node
	for (edge_idx_t edgeIdx : slackNode.edges)
	{
		GridEdge& edge = grid->edges[edgeIdx];
		int neighborIdx = edge.parent == slackNodeIdx ? edge.child : edge.parent;
		GridNode& neighbor = grid->nodes[neighborIdx];

		complex_t y = (complex_t)1 / edge.z_c;
		yv -= neighbor.v * y;
		ySum += y;
	}
	yv += slackNode.v * ySum; // Add admittance term
	slackNode.s = slackNode.v * std::conj(yv); // Compute complex power
}