set(CMAKE_POSITION_INDEPENDENT_CODE ON)
cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_CXX_STANDARD 17)

project(PowerFlow VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
enable_testing()

# Add compiler flags conditionally
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /FS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(CODE_COVERAGE)
    add_compile_options(-O0 -g --coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
endif()

add_subdirectory(src)
add_subdirectory(standalone)
add_subdirectory(tests)
 
find_package(Matlab)

if(Matlab_FOUND)
    message(STATUS "Matlab found: Adding matlab subdirectory.")
    add_subdirectory(matlab)
else()
    message(WARNING "Matlab not found: Skipping matlab subdirectory.")
endif()

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11)

if(pybind11_FOUND)
    message(STATUS "Pybind found: Adding python subdirectory.")
    add_subdirectory(python)
else()
    message(WARNING "Pybind11 not found: skipping python subdirectory")
endif()
