Reading measurement data from CSV files
=======================================

PowerFlow supports reading measurement data from CSV files and using it to identify
incorrect cable parameters in the network.

Required input files
--------------------

Two CSV files are required:

- A **node data file** containing connections, voltage IDs and current at each connection.
- A **voltage file** containing voltages for end nodes by ID, where each row represents
  a measurement timestamp.

Configuration
-------------

The relevant columns and file names are configured in ``read_data.py`` using the following
variables:

.. code-block:: python

   ### FILE NAMES ###
   VOLTAGE_FILE = SETTINGS.VOLTAGE_FILE
   NODE_DATA_FILE = SETTINGS.NODE_DATA_FILE

   ### FILE INDEXES ###
   NODE_CONNECTIONS = SETTINGS.NODE_CONNECTIONS
   START_NODE_INDEX = SETTINGS.START_NODE_INDEX
   END_NODE_INDEX = SETTINGS.END_NODE_INDEX
   PMAX_CONNECTION = SETTINGS.PMAX_CONNECTION

   VOLTAGE_L1 = SETTINGS.VOLTAGE_L1
   VOLTAGE_L2 = SETTINGS.VOLTAGE_L2
   VOLTAGE_L3 = SETTINGS.VOLTAGE_L3

   ### GLOBAL PREFIX ###
   PREFIX = SETTINGS.PREFIX

Running
-------

Once the configuration is set, run the data reading script with:

.. code-block:: bash

   python3 read_data.py