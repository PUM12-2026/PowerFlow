from typing import Any, List

import PowerFlowPython
import os


def check_list_types(data: List[Any], name: str, expected_len=None):
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
        data=power_flow.getAllVoltages(), name="all_voltages", expected_len=10
    )
    check_list_types(
        data=power_flow.getLoadVoltages(), name="load_voltages", expected_len=3
    )
    check_list_types(
        data=power_flow.getSlackPowers(), name="slack_powers", expected_len=3
    )
    check_list_types(data=power_flow.getCurrents(), name="currents", expected_len=7)


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
    check_list_types(data=voltages, name="reset_all_voltages", expected_len=10)
    check_list_types(data=load_voltages, name="reset_load_voltages", expected_len=3)
    check_list_types(data=currents, name="reset_currents", expected_len=7)
    check_list_types(data=slack_powers, name="reset_slack_powers", expected_len=3)

    # Make sure the voltages are indeed reset.
    for v in voltages + load_voltages:
        assert v == complex(1.0, 0.0), f"Voltage {v} did not reset to 1.0+0j."

    # Make sure the powers and currents are indeed reset.
    for x in currents + slack_powers:
        assert x == complex(0.0, 0.0), f"Value {x} did not reset to 0.0+0j."


def test_powerflow_bindings():
    """
    An integration test to verify that the Python bindings correctly interface
    with the PowerFlow C++ implementation via Pybind11 and that the reset()
    command properly clears the solver state.
    """

    settings = PowerFlowPython.SolverSettings()
    settings.max_iterations_total = 1000

    network_path = "examples/test_networks/test_network.txt"
    if not os.path.exists(network_path):
        print(f"Skipping test: {network_path} not found.")
        return

    power_flow = PowerFlowPython.PowerFlow(network_path, settings)

    # Define complex power (S) and a reference voltage (V)
    S = [complex(-0.004, -0.002), complex(-0.002, -0.001), complex(-0.005, -0.004)]
    V = [complex(1.0, 0.0)]

    power_flow.solve(S, V)
    verify_solve_state(power_flow)

    power_flow.reset()
    verify_reset_state(power_flow)

    print("Python binding test passed!")


if __name__ == "__main__":
    test_powerflow_bindings()
