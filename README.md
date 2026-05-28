# PowerFlow

PowerFlow is a [power flow/load flow](https://en.wikipedia.org/wiki/Power-flow_study) simulation tool that can be used to calculate voltages, gradients, and cable parameters in electrical grids given known
power consumptions. PowerFlow is written in C++ for fast performance and comes with Matlab
and Python bindings for use within Matlab and Python scripts. 

## Documentation

Full documentation is available at: https://powerflow-liu.readthedocs.io/en/latest/

The documentation covers:
- Installation and build instructions for Matlab, Python and C++
- General concepts such as node types, per-unit values and network structure
- How to define networks using network files
- Basic usage including how to solve a network and retrieve voltages and currents
- Computing gradients and performing cable parameter estimation
- Information for developers including unit tests and project structure

## Quick start

To build PowerFlow you need CMake and a C++17 compiler. See the 
[documentation](https://powerflow-liu.readthedocs.io/en/latest/) for detailed 
build and usage instructions.
