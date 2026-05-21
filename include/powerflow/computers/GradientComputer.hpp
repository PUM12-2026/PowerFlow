#ifndef POWERFLOW_GRADIENT_COMPUTER_H
#define POWERFLOW_GRADIENT_COMPUTER_H

#include "powerflow/network.hpp"

#include <Eigen/Dense>
#include <Eigen/Core>
#include <complex>
#include <vector>

/**
 * @class GradientComputer
 * @brief Handles sensitivity computations for Power Flow.
 */
class GradientComputer
{
public:
    /**
     * @brief Computes the gradient, dSloss/dS, by using the chain rule. Where
     * Sloss = Ploss + jQloss, S = P + jQ, V = Vre + jVim, and I = Ire + jIim.
     *
     * This results in the following matrix product:
     * ```
     * [dPloss/dP dPloss/dQ]   [dPloss/dIre dPloss/dIim]   [dIre/dP dIre/dQ]
     * [dQloss/dP dQloss/dQ] = [dQloss/dIre dQloss/dIim] * [dIim/dP dIim/dQ]
     * ```
     * @param v    Complex voltage (V) V = Vre + jVim.
     * @param i    Complex current (A) I = Ire + jIim.
     * @param z    Complex impedance (Ohm) Z = R + jX.
     * @param dVdS Matrix dV/dS: Jacobian of (Vre, Vim) w.r.t. (P, Q).
     * @param dIdS Matrix dI/dS: Jacobian of (Ire, Iim) w.r.t. (P, Q).
     */
    std::vector<Eigen::Matrix2d> computeDsLossDs(
        complex_t v, complex_t i, complex_t z,
        const std::vector<Eigen::Matrix2d> &dVdS,
        const std::vector<Eigen::Matrix2d> &dIdS,
        size_t numLeaves //
    );

    /**
     * @brief Computes the total derivative, dI/dS, by using the chain rule. Where
     * I = Ire + jIim, S = P + jQ, and V = Vre + jVim.
     *
     * This results in the following matrix product:
     * ```
     * [dIre/dP dIre/dQ]   [dIre/dP_local dIre/dQ_local]   [dP_local/dP dP_local/dQ]   [dIre/dVre dIre/dVim]   [dVre/dP dVre/dQ]
     * [dIim/dP dIim/dQ] = [dIim/dP_local dIim/dQ_local] * [dQ_local/dP dQ_local/dQ] + [dIim/dVre dIim/dVim] * [dVim/dP dVim/dQ]
     * ```
     * @param nodeV    Complex voltage V (V) = Vre + jVim.
     * @param nodeS    Complex power (W) S = P + jQ.
     * @param dSdSNode Matrix dS/dS: Jacobian of local (P, Q) w.r.t. leaf (P, Q).
     * @param dVdSNode Matrix dV/dS: Jacobian of (Vre, Vim) w.r.t. leaf (P, Q).
     */
    std::vector<Eigen::Matrix2d> computeDiDs(
        complex_t nodeV,
        complex_t nodeS,
        const std::vector<Eigen::Matrix2d> &dSdSNode,
        const std::vector<Eigen::Matrix2d> &dVdSNode //
    );

    /**
     * @brief Computes the total derivative, dV/dS. Where V = Vre + jVim, S = P + jQ,
     * I = Ire + jIim, and Z = R + jX.
     *
     * This results in the following matrix sum:
     * ```
     * [dVre/dP dVre/dQ]   [dVre_parent/dP dVre_parent/dQ]   [dVre/dIre dVre/dIim]   [dIre/dP dIre/dQ]
     * [dVim/dP dVim/dQ] = [dVim_parent/dP dVim_parent/dQ] + [dVim/dIre dVim/dIim] * [dIim/dP dIim/dQ]
     * ```
     * @param dVdSParent Matrix dV/dS of the parent node.
     * @param dIdSNode   Matrix dI/dS of the branch current.
     * @param z          Complex impedance (Ohm) Z = R + jX.
     * @param numLeaves  Number of leaf nodes.
     */
    std::vector<Eigen::Matrix2d> computeDvDs(
        const std::vector<Eigen::Matrix2d> &dVdSParent,
        const std::vector<Eigen::Matrix2d> &dIdSNode,
        complex_t z,
        size_t numLeaves //
    );

    /**
     * @brief Computes the upstream power gradient contribution from a single branch.
     * Adds the power loss sensitivity of the branch to the total downstream node sensitivity.
     *
     * This results in the following matrix addition for each leaf node m:
     * ```
     * [dP_parent/dP_m dP_parent/dQ_m]   [dP_child/dP_m dP_child/dQ_m]   [dPloss/dP_m dPloss/dQ_m]
     * [dQ_parent/dP_m dQ_parent/dQ_m] = [dQ_child/dP_m dQ_child/dQ_m] + [dQloss/dP_m dQloss/dQ_m]
     * ```
     * @param dSChild     Vector of dS/dS matrices from the child node.
     * @param dSlossChild Vector of dSloss/dS matrices from the branch connecting to the child node.
     */
    std::vector<Eigen::Matrix2d> computeDsDs(
        const std::vector<Eigen::Matrix2d> &dSChild,
        const std::vector<Eigen::Matrix2d> &dSlossChild //
    );

private:
    /**
     * @brief Computes the partial derivative, dSloss/dI, by using the
     * definition of Sloss = 3 * Z * |I|^2. Where Sloss = Ploss + jQloss,
     * Z = R + jX and I = Ire + jIim.
     *
     * This results in the following matrix:
     * ```
     * [dPloss/dIre dPloss/dIim]       [R*Ire R*Iim]
     * [dQloss/dIre dQloss/dIim] = 6 * [X*Ire X*Iim]
     * ```
     * @param i Complex current (A) I = Ire + jIim.
     * @param z Complex impedance (Ohm) Z = R + jX.
     */
    Eigen::Matrix2d computeDsLossDi(complex_t i, complex_t z);

    /**
     * @brief Computes the partial derivative, dI/dV, by using the definition
     * of I = conj(S / (√3 * V)). Where I = Ire + jIim, V = Vre + jVim, and S = P + jQ.
     *
     * @param nodeV Complex voltage (V) V = Vre + jVim.
     * @param nodeS Complex power (W) S = P + jQ.
     */
    Eigen::Matrix2d computeDiDv(complex_t nodeV, complex_t nodeS);

    /**
     * @brief Computes the partial derivative, dV/dI, by using the
     * definition of V = V_0 - √3 * I * Z. Where V = Vre + jVim,
     * Z = R + jX and I = Ire + jIim.
     *
     * This results in the following matrix:
     * ```
     * [dVre/dIre dVre/dIim]        [-R   X]
     * [dVim/dIre dVim/dIim] = √3 * [-X  -R]
     * ```
     * @param z Complex impedance (Ohm) Z = R + jX.
     */
    Eigen::Matrix2d computeDvDi(complex_t z);
};

#endif