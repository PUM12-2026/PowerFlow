Computing Gradients
===================

PowerFlow supports computing gradients of currents, voltages and power with respect to
the power consumption at LOAD nodes. This can be used to analyze how changes in household
power consumption affect the rest of the network.

.. note::

   Gradient computation requires a fully radial network (i.e. a network with no cycles).
   If the network contains cycles, gradients will not be computed.

To enable gradient computation, set ``compute_gradients = True`` in the settings object
before loading the network.

Available gradient functions
-----------------------------

- ``getdIdS()`` – returns the current of each cable with respect to the household effects.
- ``getdVdS()`` – returns the voltage of every node except the slack node with respect to the household effects.
- ``getDsDs()`` – returns the effect of every node except household with respect to the household effects.
- ``getDslossDs()`` – returns the effect loss of each cable with respect to the household effects.

Python example
--------------

.. code-block:: python

   import PowerFlowPython

   # Define the settings object.
   settings = PowerFlowPython.SolverSettings()

   # Max number of iterations for the entire network.
   settings.max_iterations_total = 1000

   # Enable computing of gradients, if not set to True none will be computed.
   settings.compute_gradients = True

   # Create a PowerFlow reference that loads the network.
   power_flow = PowerFlowPython.PowerFlow("examples/test_networks/test_network_single_grid.txt", settings)

   # Define household node's power in the order of the households 5, 6 and 7.
   S = [complex(-0.004, -0.002), complex(-0.002, -0.001), complex(-0.005, -0.004)]

   # Define slack node's voltage.
   V = [complex(1.0, 0.0)]

   # Solve the network using PowerFlow.
   power_flow.solve(S, V)

   # Get the current of each cable with respect to the household effects.
   dIdS = power_flow.getdIdS()

   # Get the voltage of every node except the slack node
   # with respect to the household effects.
   dVdS = power_flow.getdVdS()

   # Get the effect of every node except household with
   # respect to the household effects.
   dSdS = power_flow.getDsDs()

   # Get the effect loss of each cable with respect to the household effects.
   dSlossDs = power_flow.getDslossDs()

Matlab example
--------------

.. code-block:: matlab

   % Define the settings object.
   settings = struct();

   % Max number of iterations for the entire network.
   settings.max_iterations_total = 1000;

   % Enable computing of gradients, if not set to True none will be computed.
   settings.compute_gradients = True;

   % Create a PowerFlow reference that loads the network.
   power_flow = PowerFlow("examples/test_networks/test_network_single_grid.txt", settings);

   % Define household node's power in the order of the households 5, 6 and 7.
   S = [complex(-0.004, -0.002), complex(-0.002, -0.001), complex(-0.005, -0.004)];

   % Define slack node's voltage.
   V = [complex(1.0, 0.0)];

   % Solve the network using PowerFlow.
   power_flow.solve(S, V);

   % Get the current of each cable with respect to the household effects.
   dIdS = power_flow.getdIdS();

   % Get the voltage of every node except the slack node
   % with respect to the household effects.
   dVdS = power_flow.getdVdS();

   % Get the effect of every node except household with
   % respect to the household effects.
   dSdS = power_flow.getDsDs();

   % Get the effect loss of each cable with respect to the household effects.
   dSlossDs = power_flow.getDslossDs();