#include "powerflow/computers/GradientComputer.hpp"

std::vector<Eigen::Matrix2d> GradientComputer::computeDsLossDs(
    complex_t v, complex_t i, complex_t z,
    const std::vector<Eigen::Matrix2d> &dVdS,
    const std::vector<Eigen::Matrix2d> &dIdS,
    size_t numLeaves //
)
{
    std::vector<Eigen::Matrix2d> result(numLeaves);

    // Compute the local sensitivity of the cable, when the current changes and when the voltage is fix.
    Eigen::Matrix2d dSLossdI = computeDsLossDi(i, z);

    for (size_t j = 0; j < numLeaves; ++j)
    {
        result[j] = dSLossdI * dIdS[j];
    }

    return result;
}

std::vector<Eigen::Matrix2d> GradientComputer::computeDiDs(
    complex_t nodeV,
    complex_t nodeS,
    const std::vector<Eigen::Matrix2d> &dSdSNode,
    const std::vector<Eigen::Matrix2d> &dVdSNode //
)
{
    std::vector<Eigen::Matrix2d> result(dSdSNode.size());

    double coefficent = 1.0 / (SQRT3 * std::norm(nodeV));

    Eigen::Matrix2d dIdS{
        {coefficent * nodeV.real(), coefficent * nodeV.imag()},
        {coefficent * nodeV.imag(), coefficent * -nodeV.real()},
    };

    Eigen::Matrix2d dIdV = computeDiDv(nodeV, nodeS);

    for (size_t i = 0; i < dSdSNode.size(); ++i)
    {
        result[i] = dIdS * dSdSNode[i] + dIdV * dVdSNode[i];
    }

    return result;
}

Eigen::Matrix2d GradientComputer::computeDiDv(complex_t nodeV, complex_t nodeS)
{
    double coefficent = 1.0 / (SQRT3 * std::norm(nodeV) * std::norm(nodeV));
    double dIreDVre = (nodeS.real() * std::norm(nodeV) - 2 * nodeV.real() * (nodeS.real() * nodeV.real() + nodeS.imag() * nodeV.imag()));
    double dIreDVim = (nodeS.imag() * std::norm(nodeV) - 2 * nodeV.imag() * (nodeS.real() * nodeV.real() + nodeS.imag() * nodeV.imag()));
    double dIimDVre = (-1 * nodeS.imag() * std::norm(nodeV) - 2 * nodeV.real() * (nodeS.real() * nodeV.imag() - nodeS.imag() * nodeV.real()));
    double dIimDVim = (nodeS.real() * std::norm(nodeV) - 2 * nodeV.imag() * (nodeS.real() * nodeV.imag() - nodeS.imag() * nodeV.real()));

    Eigen::Matrix2d dIdV{
        {coefficent * dIreDVre, coefficent * dIreDVim},
        {coefficent * dIimDVre, coefficent * dIimDVim},
    };

    return dIdV;
}

std::vector<Eigen::Matrix2d> GradientComputer::computeDvDs(
    const std::vector<Eigen::Matrix2d> &dVdSParent,
    const std::vector<Eigen::Matrix2d> &dIdSNode,
    complex_t z,
    size_t numLeaves // ,
)
{
    std::vector<Eigen::Matrix2d> result(numLeaves);

    // Compute the voltage-to-current sensitivity matrix for this branch
    Eigen::Matrix2d dVdI = computeDvDi(z);

    for (size_t i = 0; i < numLeaves; ++i)
    {
        // Total Sensitivity = Parent Sensitivity + (Branch Impedance Matrix * Current Sensitivity)
        result[i] = dVdSParent[i] + (dVdI * dIdSNode[i]);
    }

    return result;
}

std::vector<Eigen::Matrix2d> GradientComputer::computeDsDs(
    const std::vector<Eigen::Matrix2d> &dSChild,
    const std::vector<Eigen::Matrix2d> &dSlossChild //
)
{
    std::vector<Eigen::Matrix2d> dSparent(dSChild.size());

    for (size_t i = 0; i < dSChild.size(); ++i)
    {
        dSparent[i] = dSChild[i] + dSlossChild[i];
    }

    return dSparent;
}

Eigen::Matrix2d GradientComputer::computeDsLossDi(complex_t i, complex_t z)
{
    Eigen::Matrix2d dSLossdI{
        {z.real() * i.real(), z.real() * i.imag()},
        {z.imag() * i.real(), z.imag() * i.imag()},
    };
    return 6.0 * dSLossdI;
}

Eigen::Matrix2d GradientComputer::computeDvDi(complex_t z)
{
    Eigen::Matrix2d dVdI{
        {-z.real(), z.imag()},
        {-z.imag(), -z.real()},
    };

    return SQRT3 * dVdI;
}