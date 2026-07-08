Basic Usage
===========

The following section describes how to use PowerFlow using the Matlab or Pyhton Interface. It covers how to perform basic power flow calculations in
electrical networks. Including loading a network, solving it, and retrieving the
results such as node voltages, edge currents and slack powers.

Using PowerFlow in a Matlab script
-----------------------------------

.. note::

   On Ubuntu and possibly other Linux distributions, Matlab may need to be started using
   a command similar to ``LD_PRELOAD=/lib/x86_64-linux-gnu/libstdc++.so.6 matlab``.

You need the MEX file as well as the ``PowerFlow.m`` script in your Matlab PATH to use
PowerFlow in Matlab scripts. See the build instructions on how to acquire those.

Loading a network
~~~~~~~~~~~~~~~~~

.. code-block:: matlab

   net = PowerFlow("path/to/network.txt", []);

The returned ``net`` variable is a pointer/handle to the loaded network. The network will
be automatically garbage collected once the handle goes out of scope.

Solve
~~~~~

.. code-block:: matlab

   net.solve(S, V);

``net.solve`` performs the power flow calculation. S, V and V_res are **complex** row
vectors. The S vector must contain one complex value per LOAD node in the network.

In a network with *n* grids, each containing *m_1* to *m_n* number of LOAD nodes, the
first *m_1* values in the S vector correspond to the load nodes in the first grid, the
second *m_2* values correspond to the load nodes in the second grid and so on. The LOAD
nodes are in turn ordered by their ID, i.e., a LOAD node with ID 0 comes before LOAD node
with ID 3 in the S vector. In the same way, V must contain one complex value per SLACK
node in the network.

Get LOAD node voltages
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: matlab

   V_load = net.getLoadVoltages();

``net.getLoadVoltages`` returns the calculated voltages at the LOAD nodes. The V_load
vector has the same format as the S vector passed to ``net.solve``.

Get all node voltages
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: matlab

   V_all = net.getAllVoltages();

Get edge currents
~~~~~~~~~~~~~~~~~

.. code-block:: matlab

   I = net.getCurrents();

The elements in the I vector are the currents in the edges in the same order they appear
in the Network file. In a network with *n* grids, each containing *m_1* to *m_n* number
of edges, the first *m_1* values in the I vector correspond to the edges in the first
grid, the second *m_2* values correspond to the edges in the second grid and so on.

Get SLACK/SLACK_IMPLICIT node voltages
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: matlab

   S_slack = net.getSlackPowers();

Reset network
~~~~~~~~~~~~~

.. code-block:: matlab

   net.reset();

All voltages will be set to 1 and all powers will be set to 0.

Passing solver options
~~~~~~~~~~~~~~~~~~~~~~

It is possible to pass additional options to the solver using a settings struct:

.. code-block:: matlab

   % Max number of iterations for the entire network.
   settings.max_iterations_total = 10000;

   % Max number of iterations for the Gauss-Seidel solver.
   settings.max_iterations_gauss = 100000;

   % Largest acceptable power mismatch for the Gauss-Seidel solver.
   settings.gauss_seidel_precision = 1e-10;

   % Max number of iterations for the Backward-Forward-Sweep solver.
   settings.max_iterations_bfs = 10000;

   % Largest acceptable power mismatch for the Backward-Forward-Sweep solver.
   settings.bfs_precision = 1e-10;

   % Max number of iterations for the ZBus Jacobi solver.
   settings.max_iterations_zbusjacobi = 10000;

   % Largest acceptable power mismatch for the ZBus Jacobi solver.
   settings.zbusjacobi_precision = 1e-10;

   % Whether to print additional information to the console
   settings.verbose_logging = false;

   net = PowerFlow("path/to/network.txt", settings);

Thread safety
~~~~~~~~~~~~~

- It is **NOT** safe to load networks simultaneously on different threads!
- It is **NOT** safe to execute ``net.solve()`` simultaneously on different threads using the same network handle! However, it is safe to simultaneously execute ``solve()`` using *different* network handles.

Using PowerFlow in a Python script
------------------------------------

You need to import the PowerFlowPython module to use PowerFlow in Python scripts. See the
build instructions on how to acquire the Python module file.

Below is a simple Python script example. For detailed usage instructions, see the Matlab
section above.

.. code-block:: python

   import PowerFlowPython

   settings = PowerFlowPython.SolverSettings()
   net = PowerFlowPython.PowerFlow("path/to/grid.txt", settings)

   net.solve(V, S)
   V_loads = net.getLoadVoltages()