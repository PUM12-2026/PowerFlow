from unittest.mock import patch
import pandas as pd
import unittest
import sys
import os

tests_dir = os.path.dirname(os.path.abspath(__file__))
misc_dir = os.path.dirname(tests_dir)
if misc_dir not in sys.path:
    sys.path.insert(0, misc_dir)

import read_data


class TestReadDataUnit(unittest.TestCase):

    @patch("read_data.VOLTAGE_FILE", "data.txt")
    @patch("sys.exit")
    @patch("builtins.print")
    def test_is_csv_non_csv_file(self, mock_print, mock_exit):
        """
        U-3.1: Test what happens if it doesn't receive a csv file.
        """

        mock_exit.side_effect = SystemExit
        with self.assertRaises(SystemExit):
            read_data.main()

        mock_print.assert_called_with("Error: Voltage file is not in CSV format.")
        mock_exit.assert_called_with(1)

    @patch("read_data.pd.read_csv")
    def test_empty_csv_file(self, mock_read_csv):
        """
        U-3.2: Test behavior when it receives an empty csv file.
        """

        mock_read_csv.return_value = pd.DataFrame(
            columns=[
                read_data.NODE_CONNECTIONS,
                read_data.START_NODE_INDEX,
                read_data.END_NODE_INDEX,
                read_data.PMAX_CONNECTION,
            ]
        )

        df = read_data.initialize_reader()
        self.assertTrue(df.empty)

        nodes = []
        read_data.initialize_nodes(df, nodes)
        self.assertEqual(len(nodes), 0)

    @patch("read_data.pd.read_csv")
    def test_faulty_data_in_column(self, mock_read_csv):
        """
        U-3.3: Test behavior with faulty data in the expected column.
        """
        
        prefix = "1"
        key1 = prefix + read_data.VOLTAGE_A
        key2 = prefix + read_data.VOLTAGE_B
        key3 = prefix + read_data.VOLTAGE_C

        df_mock = pd.DataFrame(
            {key1: ["faulty_string", "data"], key2: ["10", "20"], key3: ["-", "-"]}
        )
        mock_read_csv.return_value = df_mock

        with self.assertRaises(ValueError):
            read_data.get_voltage(1)


if __name__ == "__main__":
    unittest.main()