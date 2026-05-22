from types import ModuleType
import unittest
import sys
import os

# powerflow/misc/tests
tests_dir = os.path.dirname(os.path.abspath(__file__))

# powerflow/misc
misc_dir = os.path.dirname(tests_dir)

if misc_dir not in sys.path:
    sys.path.insert(0, misc_dir)
if tests_dir not in sys.path:
    sys.path.insert(0, tests_dir)

# Define generic configuration mappings globally
CONFIG_MAPS = {
    "NODE_CONNECTIONS": "Conn_ID",
    "START_NODE_INDEX": "Node_S",
    "END_NODE_INDEX": "Node_E",
    "PMAX_CONNECTION": "P_Max",
    "VOLTAGE_A": "_F1",
    "VOLTAGE_B": "_F2",
    "VOLTAGE_C": "_F3",
    "PREFIX": "ID_",
    "VOLTAGE_FILE": "",
    "NODE_DATA_FILE": "",
}

# Initialize the SETTINGS module mock with the generic configurations
if "SETTINGS" not in sys.modules:
    settings_mock = ModuleType("SETTINGS")
    for prop, value in CONFIG_MAPS.items():
        setattr(settings_mock, prop, value)
    sys.modules["SETTINGS"] = settings_mock

# Import read_data and bind the properties directly to it as well
import read_data

for prop, value in CONFIG_MAPS.items():
    setattr(read_data, prop, value)


def run_all_tests():
    loader = unittest.TestLoader()
    suite = loader.discover(start_dir=tests_dir, pattern="test_read_data_*.py")
    runner = unittest.TextTestRunner(verbosity=2)

    # Capture the TestResult object
    result = runner.run(suite)

    # If the suite wasn't completely successful, exit with code 1 to fail the CI pipeline
    if not result.wasSuccessful():
        sys.exit(1)


if __name__ == "__main__":
    run_all_tests()