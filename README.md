# PowerFlow

PowerFlow is a [power flow/load flow](https://en.wikipedia.org/wiki/Power-flow_study) simulation tool that can be used to calculate voltages in electrical grids given known power consumptions. PowerFlow is written in C++ for fast performance and comes with Matlab and Python bindings for use within Matlab and Python scripts.

Version 1.0 of PowerFlow was written in 2025 by a group of eight students as part of the course TDDD96 "Software Engineering - Bachelor Project" at Linköping University.

## Building

To build PowerFlow for your platform, you need [CMake](https://cmake.org/) and a suitable C++17 compiler. The instructions in this section assume the following compilers are used:

- Windows: MSVC ([Visual Studio >= 2022](https://visualstudio.microsoft.com/)).
- Linux: g++.
- Mac: clang.

In addition to CMake and a compiler, you may need additional software depending on the build target.

### Matlab

PowerFlow can be compiled into a [Matlab executable (MEX)](https://se.mathworks.com/help/matlab/cpp-mex-file-applications.html) that can then be used like any other Matlab function. To be able to compile and run the MEX file, Matlab must be installed and **added to the PATH** on your computer. PowerFlow has been tested to work with Matlab version R2024b and R2025a.

To compile the MEX file, execute the following in a terminal/PowerShell inside the PowerFlow root directory:

```
mkdir build
cd build
```

On Linux/macOS, run the following:

```
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target PowerFlowMex
```

On Windows:

```
cmake ..
cmake --build . --target PowerFlowMex --config Release
```

The MEX file can then be found in `build/Matlab` (`build/Matlab/Release` on Windows). Copy the MEX file to your Matlab project along with the `PowerFlow.m` file, which provides an interface to the MEX file.

### Python

PowerFlow can be compiled into a [pybind11 module](https://github.com/pybind/pybind11) that can then be imported into Python 3 scripts. To build and use the Python module, you need Python 3 and pybind11 on your computer.

**\*NOTE:** On Windows, Python must be installed using the official installer (found [here](https://www.python.org/)), **NOT** using the Microsoft Store! During the installation, make sure to select the "Add to PATH" option.\*

**\*NOTE:** When installing pybind11 using pip, it may be necessary to select the "global" version. On some Linux distributions, pybind11 may be installed using the package manager. For example, on Ubuntu, the python3-pybind11 package can be installed using apt.\*

To compile the Python module, execute the following in a terminal/PowerShell inside the PowerFlow root directory:

```
mkdir build
cd build
```

On Linux/macOS, run the following:

```
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target PowerFlowPython
```

On Windows:

```
cmake ..
cmake --build . --target PowerFlowPython --config Release
```

The resulting Python module file can then be found in `build/python` (`build/python/Release` on Windows). This file can be copied to your Python project.

#### Local development (with Type Support)

Alternatively, run the following on any platform to include Python type support. Please ensure you first create and activate a new virtual environment using venv:

On Linux/macOS, run the following:

```
python3 -m venv .venv
source .venv/bin/activate
```

On Windows:

```
python -m venv .venv
.venv\Scripts\activate
```

##### Install and build

Once the environment is active, install the dependencies and build PowerFlow:

```
# Install requirements to compile PowerFlow
pip install -r requirements.txt

# Install PowerFlow in editable mode
pip install -e . --no-build-isolation
```

Once completed, the necessary files can be found in `python/`. The most important files generated are `PowerFlowPython.cp314-win_amd64.pyd` (or .so) and `PowerFlowPython.pyi`.

### Linking to PowerFlow

It is possible to use PowerFlow as a C++ library by statically linking to the `PowerFlowLib` CMake target. The `standalone` directory contains an example showing how PowerFlow can be used within a C++ program.

## Usage

It is strongly recommended to read the "General concepts" section below before continuing with the usage instructions. For examples of use cases, see the `examples/` directory.

### General concepts

In PowerFlow, an electrical network/power network is referred to as a _network_ consisting of one or more interconnected _grids_ that each have an associated voltage level. A grid consists of a set of _nodes_ connected by _edges_, representing the cables in the grid. A network can thus be depicted as a graph, for example:

<img src="./examples/example_network.png" width="500">

Each edge in the graph has an associated impedance (Z) and each node has an associated quantity depending on its type (see below). _Connections_ between grids (the dashed lines in the graph) represent ideal transformers.

#### Node types

A grid node is of one of the following types: LOAD, LOAD_IMPLICIT, MIDDLE, SLACK, SLACK_IMPLICIT.

- LOAD nodes are nodes where the power consumption is known and the voltage is unknown.
- LOAD_IMPLICIT nodes are similar to LOAD nodes, but the power is instead specified by a connection to another grid.
- MIDDLE nodes are nodes that are only used for branching and connections.
- SLACK nodes are nodes where the voltage is known and the power is unknown.
- SLACK_IMPLICIT nodes are similar to SLACK nodes, but the voltage is instead specified by a connection to another grid.

Note that:

- Each grid in a network must have a voltage reference. Therefore, at least one SLACK or SLACK_IMPLICIT node must exist in each grid.
- When connecting two grids, one of the nodes has to be a SLACK_IMPLICIT node and the other node has to be a LOAD_IMPLICIT node.

#### Per-unit

All calculations are performed using _per-unit_ values instead of actual units (Volt, Watt etc.). Per-unit for voltages and powers are defined as:

```

V_pu = V / V_base
S_pu = S / S_base

```

where _S_base_ and _V_base_ are positive, real-valued scale factors. Each grid in a network has an associated S_base and V_base.

#### Algorithm description

PowerFlow calculates voltages in each grid separately using different solvers (algorithms) depending on the structure of the grid (see "Solvers" below). Grid solvers are executed sequentially, starting from grid 0. When all grid solvers have been executed, the voltages and powers are transferred between all grids as specified by the connections in the network. Then, the grid solvers are executed again and so on. The algorithm stops when PowerFlow has detected that all grid solutions have converged within 0 iterations.

When a network is loaded, all voltages are initially set to (1, 0). Further calculations use the previously calculated voltages as initial guesses.

#### Solvers

PowerFlow implements three different algorithms (solvers): For grids that have a single SLACK/SLACK*IMPLICIT node, contain no cycles and where the LOAD/LOAD_IMPLICIT nodes are located at the "leaves", a \_Backward-Forward-Sweep* (BFS) algorithm is used. For other grids, the _ZBus Jacobi_ or _Gauss-Seidel_ algorithm is used. _ZBus Jacobi_ can only be used when a grid contains a single SLACK/SLACK_IMPLICIT node and when the grid doesn't contain too many nodes. PowerFlow automatically detects which solver is most suitable for each grid.

#### Limitations

Some limitations on the structure of a network are imposed by PowerFlow:

- It is not possible to have more than one edge between the same pair of nodes.
- It is not possible to have disjointed grids.
- Each grid must have at least one SLACK/SLACK_IMPLICIT node. Note that some solvers only accept a single SLACK/SLACK_IMPLICIT node.
- The same SLACK_IMPLICIT/LOAD_IMPLICIT nodes can only be used in one connection.
- Impedances cannot be 0.

### Network files

A _network file_ is a text file that describes a network in a simple format that PowerFlow understands. The syntax of network files is best described by an example. The following example defines the network illustrated in the "General concepts" section:

```

# Lines beginning with # are ignored.

# The "grid" command indicates the start of a grid description.

# Since this is the first grid in the file, it will get ID 0.

grid

# S_base followed by V_base.

1000000000 10000

# Grid cables (edges) on the format: <start node> <end node> <impedance>.

# The % sign marks end of list.

0 1 (0.05, 0.05)
1 2 (0.05, 0.05)
1 3 (0.05, 0.05)
%

# Node types on the format: <node> <type>.

# <type> can be either s (SLACK), si (SLACK_IMPLICIT), l (LOAD)

# or li (LOAD_IMPLICIT).

# Nodes not listed here will automatically become MIDDLE nodes.

# The % sign marks end of list.

0 s
2 li
3 li
%

# Grid 1.

grid
10000000 400
0 1 (0.02, 0.02)
1 2 (0.05, 0.03)
%
0 si
2 l
%

# Grid 2.

grid
10000000 400
0 1 (0.02, 0.03)
0 2 (0.03, 0.03)
%
0 si
1 l
2 l
%

# Connections between grids on the format:

# <"upper" grid> <LOAD_IMPLICIT node in "upper" grid> <"lower" grid> <SLACK_IMPLICIT node in "lower" grid>

# The % sign marks end of list.

connections
0 2 1 0
0 3 2 0
%

```

### Using PowerFlow in a Matlab script

**\*NOTE:** On Ubuntu and possibly other Linux distributions, Matlab may need to be started using a command similar to `LD_PRELOAD=/lib/x86_64-linux-gnu/libstdc++.so.6 matlab`.\*

You need the MEX file as well as the PowerFlow.m script in your Matlab PATH to use PowerFlow in Matlab scripts. See the build instructions on how to acquire those.

#### Loading a network

```

net = PowerFlow("path/to/network.txt", []);

```

The returned `net` variable is a pointer/handle to the loaded network. The network will be automtically garbage collected once the handle goes out of scope.

#### Solve

```

net.solve(S, V);

```

`net.solve` performs the power flow calculation. S, V and V_res are **complex** row vectors. The S vector must contain one complex value per LOAD node in the network.

In a network with _n_ grids, each containing _m_1_ to _m_n_ number of LOAD nodes, the first _m_1_ values in the S vector correspond to the load nodes in the first grid, the second _m_2_ values correspond to the load nodes in the second grid and so on. The LOAD nodes are in turn ordered by their ID, i.e., a LOAD node with ID 0 comes before LOAD node with ID 3 in the S vector. In the same way, V must contain one complex value per SLACK node in the network.

#### Get LOAD node voltages

```

V_load = net.getLoadVoltages();

```

`net.getLoadVoltages` returns the calculated voltages at the LOAD nodes. The V_load vector has the same format as the S vector passed to `net.solve`.

#### Get all node voltages

```

V_all = net.getAllVoltages();

```

#### Get edge currents

```

I = net.getCurrents();

```

The elements in the I vector are the currents in the edges in the same order they appear in the Network file. In a network with _n_ grids, each containing _m_1_ to _m_n_ number of edges, the first _m_1_ values in the I vector correspond to the edges in the first grid, the second _m_2_ values correspond to the edges in the second grid and so on.

#### Get SLACK/SLACK_IMPLICIT node voltages

```

S_slack = net.getSlackPowers();

```

#### Reset network

```

net.reset();

```

All voltages will be set to 1 and all powers will be set to 0.

#### Passing solver options

It is possible to pass additional options to the solver using a settings struct:

```

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

net = PowerFlow("path/to/network.txt", settings);

```

#### Thread safety

- It is **NOT** safe to load networks simultaneously on different threads!
- It is **NOT** safe to execute net.solve() simultaneously on different threads using the same network handle! However, it is safe to simultaneously execute solve() using _different_ network handles.

### Using PowerFlow in a Python script

You need to import the PowerFlowPython module to use PowerFlow in Python scripts. See the build instructions on how to acquire the Python module file.

Below is a simple Python script example. For detailed usage instructions, see the Matlab section above.

```

import PowerFlowPython

settings = PowerFlowPython.SolverSettings()
net = PowerFlowPython.PowerFlow("path/to/grid.txt", settings)

net.solve(V, S)
V_loads = net.getLoadVoltages()

```

## For developers

### Unit tests

To run the unit test build the project and run ./build/tests/UnitTests

Depending on compiler you can run in to problems with the path to the test networks, if this happens replace the localPath variable with the path to the repository.

### Project structure

- `examples/` - Various examples.
- `include/powerflow/` - Public headers.
- `matlab/` - Matlab implementation.
- `scripts/` - Miscellaneous files.
- `python/` - Python implementation.
- `src/` - powerflow source code.
- `standalone/` - Standalone executable.
- `tests/` - Unit tests.

```

```
