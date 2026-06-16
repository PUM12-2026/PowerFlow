Performing Cable Parameter Estimation
=====================================

Powerflow supports estimation of cable parameters using measurement data and network topology.

.. note::

   Cable parameter estimation requires a fully radial network (i.e. a network with no cycles).
   If the network contains cycles, cable parameters will not be computed.

PowerFlow has two methods for performing cable parameter estimation. Both require measurements at the slack node and at all load nodes.

Regression Method
-----------------

This method requires multiple time samples. The larger the network the more are required, but generally the more samples are passed in the better. First a ``PowerFlowSolver`` object must be created, then the ``solveParamsReg`` method must be called, passing in slack voltages, load voltages, and load power consumptions, as well as a convergence threshold and a max number of iterations, as the method is iterative.

Matlab example:

.. code-block:: matlab

   % A PowerFlow object pf must be created
   % (...)

   % List of load node indexes
   keys = [3, 5, 6];

   % List of voltages, t for each load node, where t is the number of samples. These must correspond to the keys above
   V = zeros(3, t);
   % List of power injections, t for each load node
   S = zeros(3, t);

   % Only the first sample is shown here, for a full example of data see matlab/tests/regression_data.m
   V(1, :) = [complex(1, 0), (...)];
   S(1, :) = [complex(0.9, 0.01), (...)];
   V(2, :) = [complex(0.95, -0.05), (...)];
   S(2, :) = [complex(1, 0), (...)];
   V(3, :) = [complex(1.1, 0.06), (...)];
   S(3, :) = [complex(1.05, 0.05), (...)];

   % List of t slack voltages
   slack = [complex(1.15, 0), (...)];

   % Call the solveParamsReg function passing in keys, voltages, power consumptions, slack voltages, 
   % convergence threshold, and a max number of iterations
   pf.solveParamsReg(keys, V, S, slack, 1e-3, 20);

Python example:

.. code-block:: python

   # A PowerFlowSolver object pfs must be created
   # (...)

   # Dictionary of loads-voltages. Each load has t samples. Only the first sample is shown here.
   V = {
      3: [[1, 0], (...)], 
      5: [[0.95, -0.05], (...)],
      6: [[1.1, 0.06], (...)]   
   }

   # Dictionary of loads-power consumptions. Same principle as the voltages.
   S = {
      3: [[0.9, 0.01], (...)], 
      5: [[1, 0], (...)],
      6: [[1.05, 0.05], (...)]  
   }

   slack = [[1.15, 0], (...)]

   # Call solveParamsReg passing in voltages, power consumptions, slack voltages, 
   # convergence threshold, and a max number of iterations
   pfs.solveParamsReg(V, S, slack, 1e-3, 20)

C++ example:

.. code-block:: c++

   // A PowerFlowSolver object pfs must be created
   // (...)

   // Load voltages. t samples for each load. Only one sample is shown here. For a full example, see ``standalone.cpp``
   std::vector<std::vector<complex_t>> loadVoltages = {
      // Load 3
      { {1, 0}, (...) },
      // Load 5
      { {0.95, -0.05}, (...) },
      // Load 6
      { {1.1, 0.06}, (...) }
   };

   // Load power injections. Same principle as voltages
   std::vector<std::vector<complex_t>> loadPowers = {
      // Load 3
      { {0.9, 0.01}, (...) },
      // Load 5
      { {1, 0}, (...) },
      // Load 6
      { {1.05, 0.05}, (...) }
   };

   // Pass the above into a load-MeasuredValues map, where MeasuredValues 
   // is a struct to hold voltages and power consumptions for a load node
   std::unordered_map<node_idx_t, MeasuredValues> measuredValues;
   measuredValues[3] = { loadVoltages[0], loadPowers[0] };
   measuredValues[5] = { loadVoltages[1], loadPowers[1] };
   measuredValues[6] = { loadVoltages[2], loadPowers[2] };

   // Slack voltages. t samples.
   std::vector<complex_t> slack = { {1.15, 0}, (...) };

   // Call solveParams passing in voltages, power consumptions, slack voltages, 
   // convergence threshold, and a max number of iterations
   pfs.solveParamsReg(measuredValues, slack, 1e-3, 20);

The ``solveParamsReg`` method will estimate cable parameters in the network and update them at each iteration, even if convergence isn't achieved.

Majority Vote Method
--------------------
This method only requires one time sample. First a ``PowerFlowSolver`` object must be created, and a state estimator must be run by calling ``solve()``. Then parameter estimation can be performed by calling ``solveParams()``, and passing in load voltages and a precision. This method is not recommended, instead prefer to use the regression method described above.

Matlab example:

.. code-block:: matlab

   % A PowerFlow object pf must be created, and solve() must be called first
   % (...)

   % List of load node indexes
   keys = [3, 5, 6];

   % List of voltages, one for each load node. These must correspond to the keys above
   vals = [complex(1, 0), complex(0.95, -0.05), complex(1.1, 0.06)];

   % Call the solveParams function passing in keys, vals, and a precision
   pf.solveParams(keys, vals, 1e-3);

Python example:

.. code-block:: python

   # A PowerFlowSolver object pfs must be created, and solve() must be called first
   # (...)

   # Dictionary of loads-voltages
   voltages = {
      3: [1, 0], 
      5: [0.95, -0.05],
      6: [1.1, 0.06]   
   }

   # Call solveParams passing in voltages and a precision
   pfs.solveParams(voltages, 1e-3)

C++ example:

.. code-block:: c++

   // A PowerFlowSolver object pfs must be created, and solve() must be called first
   // (...)

   // Map of loads-voltages
   std::unordered_map<node_idx_t, complex_t> voltages = {
      {3, {1, 0}},
      {5, {0.95, -0.05}},
      {6, {1.1, 0.06}}
   };

   // Call solveParams passing in voltages and a precision
   pfs.solveParams(voltages, 1e-3);

The ``solveParams`` method will notify the user when invalid cable parameters are detected, and will adjust them when possible.

