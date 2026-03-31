import random
import typing


def generate_grid(file: typing.TextIO, grid_no: int) -> tuple[int, int]:
    # Dictionary storing the tree structure of the generated grid
    # key = parent node, value = list of child nodes

    network = {}
    grid_size = random.randint(2, 200)
    #grid_size = 35
    file.write("grid\n")
    file.write(f"# {grid_no} \n")
    file.write("10000000 400\n")

    # Generate a random tree structure
    for i in range(1, grid_size):
        parent = random.randint(0, i - 1)
        if parent in network:
            network[parent].append(i)
        else:
            network[parent] = [i]

    # Write all edges/connections in the grid to the file
    for key in network:
        for value in network[key]:
            file.write(f"{key} {value} (0.05, 0.05)\n")

    file.write("%\n")

    file.write("0 si\n")

    load_nodes = 0

    # Identify and write leaf nodes as load nodes
    for value in network.values():
        for node in value:
            if node not in network:
                file.write(f"{node} l\n")
                load_nodes += 1

    file.write("%\n")
    return (grid_size, load_nodes)


def generate_network(file_name: str, loops: bool = False):
    # Open output file for writing the full network
    
    with open(file_name, "w") as f:
        networks = random.randint(2000, 4000)
        networks = 3500
        size = 1
        loads = 0

        # Generate all sub-grids
        for i in range(0, networks):
            (grid_size, grid_loads) = generate_grid(f, i)
            size += grid_size + 1
            loads += grid_loads

        # Create a top-level grid connecting the sub-networks
        f.write("grid\n")
        f.write("1000000000 10000\n")

        # Connect each subnet to the root node or previous subnet
        for i in range(1, networks+1):
            if loops:
                # Create a chain-like structure
                f.write(f"{i} {i-1} (0.0005, 0.0005)\n")
            else:
                # Connect all subnets to node 0 (star topology)
                f.write(f"{i} {0} (0.0005, 0.0005)\n")
        
        # Close the loop if loop mode is enabled
        if loops:
            f.write(f"{0} {i} (0.0005, 0.0005)\n")

        f.write("%\n")

        # Define slack-node
        f.write("0 s\n")

        # Define load-interface nodes
        for i in range(1, networks+1):
            f.write(f"{i} li\n")

        f.write("%\n")

        # Write connection mapping between the main grid and subnets
        f.write("connections\n")
        for i in range(1, networks+1):
            f.write(f"{networks} {i} {i-1} 0\n")

        f.write("%\n")

        # Print summary of generated network
        print(f"Generated a network with {networks} subnets, {size} nodes and {loads} loads")


if __name__ == "__main__":
    generate_network("stort_natverk.txt", False)

    # generate_grid(1)
