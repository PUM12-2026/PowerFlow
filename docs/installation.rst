Installation
============

To build PowerFlow for your platform, you need `CMake <https://cmake.org/>`_ and a suitable
C++17 compiler. The instructions in this section assume the following compilers are used:

* Windows: MSVC (`Visual Studio >= 2022 <https://visualstudio.microsoft.com/>`_)
* Linux: g++
* Mac: clang

In addition to CMake and a compiler, you may need additional software depending on the
build target.

Matlab
------

PowerFlow can be compiled into a `Matlab executable (MEX) <https://se.mathworks.com/help/matlab/cpp-mex-file-applications.html>`_
that can then be used like any other Matlab function. To be able to compile and run the MEX
file, Matlab must be installed and **added to the PATH** on your computer. PowerFlow has
been tested to work with Matlab version R2024b and R2025a.

To compile the MEX file, execute the following in a terminal/PowerShell inside the PowerFlow
root directory:

.. code-block:: bash

   mkdir build
   cd build

On Linux/macOS:

.. code-block:: bash

   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . --target PowerFlowMex

On Windows:

.. code-block:: bash

   cmake ..
   cmake --build . --target PowerFlowMex --config Release

The MEX file can then be found in ``build/Matlab`` (``build/Matlab/Release`` on Windows).
Copy the MEX file to your Matlab project along with the ``PowerFlow.m`` file, which provides
an interface to the MEX file.

Python
------

PowerFlow can be compiled into a `pybind11 module <https://github.com/pybind/pybind11>`_
that can then be imported into Python 3 scripts. To build and use the Python module, you
need Python 3 and pybind11 on your computer.

.. note::

   On Windows, Python must be installed using the official installer (found
   `here <https://www.python.org/>`_), **NOT** using the Microsoft Store! During the
   installation, make sure to select the "Add to PATH" option.

.. note::

   When installing pybind11 using pip, it may be necessary to select the "global" version.
   On some Linux distributions, pybind11 may be installed using the package manager. For
   example, on Ubuntu, the python3-pybind11 package can be installed using apt.

To compile the Python module, execute the following in a terminal/PowerShell inside the
PowerFlow root directory:

.. code-block:: bash

   mkdir build
   cd build

On Linux/macOS:

.. code-block:: bash

   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build . --target PowerFlowPython

On Windows:

.. code-block:: bash

   cmake ..
   cmake --build . --target PowerFlowPython --config Release

The resulting Python module file can then be found in ``build/python``
(``build/python/Release`` on Windows). This file can be copied to your Python project.

Local development (with Type Support)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Alternatively, run the following on any platform to include Python type support. Please
ensure you first create and activate a new virtual environment using venv:

On Linux/macOS:

.. code-block:: bash

   python3 -m venv .venv
   source .venv/bin/activate

On Windows:

.. code-block:: bash

   python -m venv .venv
   .venv\Scripts\activate

Install and build
^^^^^^^^^^^^^^^^^

Once the environment is active, install the dependencies and build PowerFlow:

.. code-block:: bash

   # Install requirements to compile PowerFlow
   pip install -r requirements.txt

   # Install PowerFlow in editable mode
   pip install -e . --no-build-isolation

Once completed, the necessary files can be found in ``python/``. The most important files
generated are ``PowerFlowPython.cp314-win_amd64.pyd`` (or ``.so``) and
``PowerFlowPython.pyi``.


SCS and LAD Regression
----------------------
PowerFlow has support for performing cable parameter estimation using LAD regression. This is an optional module. If LAD regression is desired, SCS must be installed (see `SCS  installation guide <https://www.cvxgrp.org/scs/install/c.html#c-install>`__ for more detailed instructions.)

To install SCS on Windows, follow these steps:

1. Clone the SCS repository to a desired directory ``git clone https://github.com/cvxgrp/scs.git``
2. Navigate to the SCS directory ``cd scs``
3. Build SCS ``mkdir build && cd build``

  * ``cmake ..``
  * ``cmake -DBUILD_INSTALL_PREFIX:PATH="C:/path/to/install/scs" -DUSE_LAPACK=0 .``
  * ``cmake --build . --config Release``
  * ``cmake --install . --config Release``

4. Navigate to the powerflow/build folder and tell CMake where SCS is installed ``cmake -DCMAKE_PREFIX_PATH="C:/path/to/install/scs" .``

If done correctly, you should see the following message when building PowerFlow: ``-- SCS found: LAD parameter estimation enabled.``

If you get the following error while running PowerFlow from MATLAB: ``Invalid MEX-file 'C:\path\to\matlab\script\PowerFlowMex.mexw64': The specified module could not be found.`` you need to copy ``scsdir.dll`` from your ``scs/build/bin/Release`` folder to your MATLAB script folder.

Linking to PowerFlow
--------------------

It is possible to use PowerFlow as a C++ library by statically linking to the
``PowerFlowLib`` CMake target. The ``standalone`` directory contains an example showing
how PowerFlow can be used within a C++ program.
