#pragma once

// Settings that can be passed to the solver.
struct SolverSettings
{
    int max_iterations_gauss{100000};
    double gauss_seidel_precision{1e-10};
    int max_iterations_bfs{10000};
    double bfs_precision{1e-10};
    int max_iterations_zbusjacobi{10000};
    double zbusjacobi_precision{1e-10};
    int max_iterations_total{10000};
    bool compute_gradients{false};
    int max_iterations_ols{20};
    double ols_precision{1e-3};
    bool verbose_logging{false};
};