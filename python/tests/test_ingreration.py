from typing import Any, List, Union

import regression_data
import PowerFlowPython
import os


def check_list_types(data: List[Any], name: str, expected_len: Union[int, None] = None):
    """
    Verify that data is in fact a list of complex values with the expected length.
    """

    # Make sure it is indeed a list.
    assert isinstance(data, list), f"{name} should be a list, got {type(data)}."

    # Make sure it is not empty.
    assert len(data) > 0, f"{name} list is empty."

    # Make sure it is the expected length.
    if expected_len is not None:
        assert (
            len(data) == expected_len
        ), f"{name} length mismatch: expected {expected_len}, got {len(data)}."

    # Make sure the items in the list are complex.
    for i, val in enumerate(data):
        assert isinstance(
            val, complex
        ), f"Element {i} in {name} is {type(val)}, not complex."


def verify_solve_state(power_flow: PowerFlowPython.PowerFlow):
    """
    Verify that all the getters return lists with complex numbers.
    """

    print("Verifying solved state types and content...")
    check_list_types(
        data=power_flow.getAllVoltages(), name="all_voltages", expected_len=8
    )
    check_list_types(
        data=power_flow.getLoadVoltages(), name="load_voltages", expected_len=3
    )
    check_list_types(
        data=power_flow.getSlackPowers(), name="slack_powers", expected_len=1
    )
    check_list_types(data=power_flow.getCurrents(), name="currents", expected_len=7)

def get_abs_voltages(power_flow: PowerFlowPython.PowerFlow):
    """
    Get the absolute values of the voltages.
    """
    voltages = power_flow.getLoadVoltages()
    abs_voltages = {i: abs(v) for i, v in enumerate(voltages)}
    return abs_voltages


def verify_cable_parameters_majority(power_flow: PowerFlowPython.PowerFlow):
    """
    Verify that all cable paramters are correct after a solve using majority voting.
    """

    print("Verifying cable paramters...")
    check_list_types(
        data=power_flow.getImpedances(), name="impedances", expected_len=7
    )

    impedances_before = power_flow.getImpedances()

    power_flow.solveParams(get_abs_voltages(power_flow), 1e-4)

    check_list_types(
        data=power_flow.getImpedances(), name="impedances", expected_len=7
    )

    impedances_after = power_flow.getImpedances()

    for i in range(len(impedances_before)):
        assert impedances_before[i] == impedances_after[i], f"Impedances changed after solveParams, when it shouldn't have. {impedances_before[i]} != {impedances_after[i]}"

def verify_cable_parameters_regression():
    """
    Verify that all cable paramters are correct after a solve using regression.
    """

    print("Verifying cable parameters regression...")
        
    settings = PowerFlowPython.SolverSettings()

    power_flow_ref = PowerFlowPython.PowerFlow("examples/net_large_ref.txt", settings)
    power_flow_fault = PowerFlowPython.PowerFlow("examples/net_large_test.txt", settings)

    check_list_types(
        data=power_flow_ref.getImpedances(), name="impedances"
    )
    check_list_types(
        data=power_flow_fault.getImpedances(), name="impedances"
    )

    ref_impedances = power_flow_ref.getImpedances()

    power_flow_fault.solveParamsReg(regression_data.measured_voltages, regression_data.measured_loads, regression_data.slack_voltages, 3e-4, 20)

    check_list_types(
        data=power_flow_fault.getImpedances(), name="impedances"
    )

    estimated_impedances = power_flow_fault.getImpedances()

    for i in range(len(ref_impedances)):
        error = abs(ref_impedances[i] - estimated_impedances[i]) / abs(ref_impedances[i])
        assert error <= 1.1e-2, f"Impedance {i} is incorrect after parameter regression. Error {error}"


def verify_gradients_types(power_flow: PowerFlowPython.PowerFlow):
    """
    Verify that the gradient getters return lists of lists of [dPre, dQim] pairs.
    """

    print("Verifying gradient types...")

    for grad_list, name in [
        (power_flow.getDiDs(), "DiDs"),
        (power_flow.getDvDs(), "DvDs"),
        (power_flow.getDsDs(), "DsDs"),
        (power_flow.getDslossDs(), "DslossDs"),
    ]:
        assert isinstance(
            grad_list, list
        ), f"{name} should be a list of lists, got {type(grad_list)}."
        for i, row in enumerate(grad_list):
            assert isinstance(
                row, list
            ), f"Row {i} in {name} should be a list, got {type(row)}."
            for j, val in enumerate(row):
                assert isinstance(
                    val, list
                ), f"Element [{i}][{j}] in {name} should be a list [dP, dQ], got {type(val)}."
                assert (
                    len(val) == 2
                ), f"Element [{i}][{j}] in {name} should have length 2, got {len(val)}."
                assert all(
                    isinstance(x, float) for x in val
                ), f"Element [{i}][{j}] in {name} should contain floats, got {[type(x) for x in val]}."


def verify_reset_state(power_flow: PowerFlowPython.PowerFlow):
    """
    Verify that all getters are correct after a reset.
    """

    print("Verifying reset state values...")

    voltages = power_flow.getAllVoltages()
    load_voltages = power_flow.getLoadVoltages()
    currents = power_flow.getCurrents()
    slack_powers = power_flow.getSlackPowers()

    # Check all containers again, so they didn't change type.
    check_list_types(data=voltages, name="reset_all_voltages", expected_len=8)
    check_list_types(data=load_voltages, name="reset_load_voltages", expected_len=3)
    check_list_types(data=currents, name="reset_currents", expected_len=7)
    check_list_types(data=slack_powers, name="reset_slack_powers", expected_len=1)

    # Make sure the voltages are indeed reset.
    for v in voltages + load_voltages:
        assert v == complex(1.0, 0.0), f"Voltage {v} did not reset to 1.0+0j."

    # Make sure the powers and currents are indeed reset.
    for x in currents + slack_powers:
        assert x == complex(0.0, 0.0), f"Value {x} did not reset to 0.0+0j."


def verify_gradients_reset(power_flow: PowerFlowPython.PowerFlow):
    """
    Verify that the gradient getters return zero values after a reset.
    """

    print("Verifying reset gradient values...")

    for grad_list, name in [
        (power_flow.getDiDs(), "DiDs"),
        (power_flow.getDvDs(), "DvDs"),
        (power_flow.getDsDs(), "DsDs"),
        (power_flow.getDslossDs(), "DslossDs"),
    ]:
        for i, row in enumerate(grad_list):
            for j, val in enumerate(row):
                assert val == [
                    0.0,
                    0.0,
                ], f"{name}[{i}][{j}] did not reset to [0.0, 0.0]."


def test_powerflow_bindings():
    """
    An integration test to verify that the Python bindings correctly interface
    with the PowerFlow C++ implementation via Pybind11 and that the reset()
    command properly clears the solver state.
    """

    settings = PowerFlowPython.SolverSettings()
    settings.max_iterations_total = 1000
    settings.compute_gradients = True

    network_path = "examples/test_networks/test_network_single_grid.txt"
    if not os.path.exists(network_path):
        print(f"Skipping test: {network_path} not found.")
        return

    power_flow = PowerFlowPython.PowerFlow(network_path, settings)

    # Define complex power (S) and a reference voltage (V)
    S = [complex(-0.004, -0.002), complex(-0.002, -0.001), complex(-0.005, -0.004)]
    V = [complex(1.0, 0.0)]

    power_flow.solve(S, V)

    verify_solve_state(power_flow)
    verify_gradients_types(power_flow)
    verify_cable_parameters_majority(power_flow)

    power_flow.reset()
    verify_reset_state(power_flow)
    verify_gradients_reset(power_flow)

    print("Python binding test passed!")


if __name__ == "__main__":
    test_powerflow_bindings()
