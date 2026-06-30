# Define standalone executable.
add_executable(PowerFlowStandalone main.cpp)

# Link to the PowerFlowLib target.
target_link_libraries(PowerFlowStandalone PRIVATE PowerFlowLib)

# Copy the example_network_single_grid.txt file from examples/ to the build
# directory. Makes it easier to run the executable without specifying full
# path to the network file.
add_custom_command(
        TARGET PowerFlowStandalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/examples/example_network_single_grid.txt
                ${CMAKE_CURRENT_BINARY_DIR}/example_network_single_grid.txt)
add_custom_command(
        TARGET PowerFlowStandalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/examples/net_bad_param.txt
                ${CMAKE_CURRENT_BINARY_DIR}/net_bad_param.txt)
add_custom_command(
        TARGET PowerFlowStandalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/examples/net_good_param.txt
                ${CMAKE_CURRENT_BINARY_DIR}/net_good_param.txt)
add_custom_command(
        TARGET PowerFlowStandalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/examples/net_large_ref.txt
                ${CMAKE_CURRENT_BINARY_DIR}/net_large_ref.txt)
add_custom_command(
        TARGET PowerFlowStandalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/examples/net_large_test.txt
                ${CMAKE_CURRENT_BINARY_DIR}/net_large_test.txt)
add_custom_command(
        TARGET PowerFlowStandalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/examples/net_ols_small.txt
                ${CMAKE_CURRENT_BINARY_DIR}/net_ols_small.txt)
add_custom_command(
        TARGET PowerFlowStandalone POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/examples/example_network2.txt
                ${CMAKE_CURRENT_BINARY_DIR}/example_network2.txt)
