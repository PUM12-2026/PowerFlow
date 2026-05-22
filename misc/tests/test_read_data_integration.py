import unittest
import sys
import os

tests_dir = os.path.dirname(os.path.abspath(__file__))
misc_dir = os.path.dirname(tests_dir)
if misc_dir not in sys.path:
    sys.path.insert(0, misc_dir)

import read_data

BASE_DIR = os.path.dirname(os.path.abspath(__file__))


class TestReadDataIntegration(unittest.TestCase):

    def setUp(self):
        self.output_file = "output.txt"

    def tearDown(self):
        # Automatically scrub program artifacts from active execution
        if os.path.exists(self.output_file):
            os.remove(self.output_file)

    def test_valid_case_with_multi_timestamp_averaging(self):
        """
        I-1.1: Validates complete network parsing against topology files.
        Tests multi-row tracking averages, tree inheritance patterns, and zero cascades.
        """

        # Point pipeline safely to File Pair 1
        read_data.NODE_DATA_FILE = os.path.join(
            BASE_DIR, "data", "generic_node_data_topology.csv"
        )
        read_data.VOLTAGE_FILE = os.path.join(
            BASE_DIR, "data", "generic_voltage_data_topology.csv"
        )

        read_data.main()
        self.assertTrue(
            os.path.exists(self.output_file), "output.txt was not generated."
        )

        with open(self.output_file, "r") as f:
            lines = {
                line.split()[0]: line.strip() for line in f.readlines() if line.strip()
            }

        # Validation: Multi-timestamp evaluation & inheritance
        self.assertIn("3", lines)
        self.assertIn("2", lines)
        self.assertIn(
            "231.0",
            lines["2"],
            "Node 2 did not inherit the correct multi-row average (231.0V).",
        )

        # Validation: Multiple source branching average
        self.assertIn("10", lines)
        self.assertIn(
            "223.0",
            lines["10"],
            "Node 10 failed its upstream aggregate split calculation.",
        )

        # Validation: Zero cascade rule propagation
        self.assertIn("20", lines)
        self.assertTrue(
            any(val in lines["20"] for val in ["0.0", "0"]),
            "Unmapped chain failed to fall back cleanly to 0.",
        )

    def test_node_aggregation_and_resiliency_handling(self):
        """
        I-1.2: Validates network aggregation and error handling against resiliency files.
        Tests connection matrix additions and exceptions on corrupted lines.
        """
        
        # Point pipeline safely to File Pair 2
        read_data.NODE_DATA_FILE = os.path.join(
            BASE_DIR, "data", "generic_node_data_resiliency.csv"
        )
        read_data.VOLTAGE_FILE = os.path.join(
            BASE_DIR, "data", "generic_voltage_data_resiliency.csv"
        )

        read_data.main()
        self.assertTrue(
            os.path.exists(self.output_file), "output.txt was not generated."
        )

        with open(self.output_file, "r") as f:
            lines = {
                line.split()[0]: line.strip() for line in f.readlines() if line.strip()
            }

        self.assertIn(
            "60", lines, "Duplicate processing dropped target key context entirely."
        )
        node_60_output = lines["60"]

        # Validation: Exceptional strings are caught without breaking calculation flows
        self.assertIn(
            "211.0",
            node_60_output,
            "ValueErrors corrupted the node instance entirely instead of utilizing early targets.",
        )

        # Validation: Separate lines pointing to identical node targets grow structural arrays
        self.assertIn("50", node_60_output)
        self.assertIn("25.5", node_60_output)
        self.assertIn("51", node_60_output)
        self.assertIn("35.0", node_60_output)


if __name__ == "__main__":
    unittest.main()