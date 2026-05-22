from pathlib import Path
import pandas as pd
import SETTINGS
import sys

# Purpose: Get gridVar ("true" node index) for each node. We can then use this index to get the correct values from the .csv files.
# Goal: Write to a txt file for future programs to use.
# Format of txt file: 
#   nodeID Voltage [(connected node, cable power), (connected node, cable power), ...]
#   or
#   nodeID Voltage
# Values are separated by space. Each node is on a new line. Ordered by nodeID.
#

### CONSTANTS, FILE NAMES, DATA LOCATIONS IN FILES ETC ##
### Data stored in separate file for data protection

### FILE NAMES ###
VOLTAGE_FILE = SETTINGS.VOLTAGE_FILE
NODE_DATA_FILE = SETTINGS.NODE_DATA_FILE

### FILE INDEXES ###
NODE_CONNECTIONS = SETTINGS.NODE_CONNECTIONS
START_NODE_INDEX = SETTINGS.START_NODE_INDEX
END_NODE_INDEX = SETTINGS.END_NODE_INDEX
PMAX_CONNECTION = SETTINGS.PMAX_CONNECTION

# Missing PREFIX and gridVar, is used later in code
VOLTAGE_A = SETTINGS.VOLTAGE_A
VOLTAGE_B = SETTINGS.VOLTAGE_B
VOLTAGE_C = SETTINGS.VOLTAGE_C

### GLOBAL PREFIX ###
PREFIX = SETTINGS.PREFIX


# Node class
# Improves code understandability and readability.
#
class node:
    def __init__(self, node_id, voltage):
        # Node ID
        # NOTE: Not equal to gridVar. gridVar is "real" node name, not the interpreted node_id
        self.node_id = node_id
        # Average measured power for the node
        self.voltage = voltage
        # The nodes connections and the cables power
        self.power_to_child = []

    def to_string(self):
        # Funciton to help write to txt file and output validation.
        # Select format by commenting out the non desired format.

        # ALTERNATIVE ONE:
        # Format: nodeID Voltage [(connected node, cable power), (connected node, cable power), ...]
        output = f"{self.node_id} {self.voltage} {self.power_to_child}"
        
        # ALTERNATIVE TWO:
        # Format without connections: nodeID Voltage
        #output = f"{self.node_id} {self.voltage}"

        return output


def is_csv(path):
    return Path(path).suffix.lower() == ".csv"


def get_power(gridVar, df):
    max_power = df.loc[gridVar, PMAX_CONNECTION]
    return max_power


def get_voltage(Grid_Var):
    # Get voltage for each node).
    # Calculates the average voltage over time and returns it.
    # If no measurements have been made, we return NaN to indicate no voltage found
    key1 = str(Grid_Var) + VOLTAGE_A
    key2 = str(Grid_Var) + VOLTAGE_B
    key3 = str(Grid_Var) + VOLTAGE_C

    voltage_df = pd.read_csv(VOLTAGE_FILE, sep=',', usecols=[key1, key2, key3], encoding='latin-1')
    voltage_df.fillna(0, inplace=True)


    # Counter for each valid voltage value.
    counter = 0
    # Total voltage for the node. We will divide this by the counter to get the average voltage for the node.
    total = 0
    
    for i in range (0, len(voltage_df)): 
        L1 = voltage_df.loc[i, key1]
        L2 = voltage_df.loc[i, key2]
        L3 = voltage_df.loc[i, key3]

        if L1 != '-' and float(L1) > 0:
            total += float(L1)
            counter += 1
        if L2 != '-' and float(L2) > 0:
            total += float(L2)
            counter += 1
        if L3 != '-' and float(L3) > 0:
            total += float(L3)
            counter += 1

    # If counter is 0, it means that there are no valid voltage values for this node. In that case, we return 0 to avoid division by zero.
    # Returns avergage voltage for the node. If this occurs, we have errors in our data.
    if counter == 0: return 'NaN'
    return total / counter


def initialize_reader():
    # Get relevant columns from the .csv file and fill empty values with empty string. We only need these columns for our calculations, so we can ignore the rest.
    # See global variables for column names.

    return pd.read_csv(NODE_DATA_FILE, sep=';', usecols=[
                           NODE_CONNECTIONS, 
                           START_NODE_INDEX, 
                           END_NODE_INDEX, 
                           PMAX_CONNECTION], encoding='latin-1').fillna('')


def initialize_nodes(df, nodes):
    # Create list of nodes from file data.
    for num in range(0, len(df)):
        # Get relevant columns
        Grid_ID = df.loc[num, NODE_CONNECTIONS]
        node_ID = df.loc[num, END_NODE_INDEX]            

        if Grid_ID != '':
            # We have a grid ID, so we can get the voltage and power for this node.
            try:
                node_voltage = get_voltage(PREFIX + str(int((float(Grid_ID.replace(',', '.'))))))
                current_node = node(node_ID, node_voltage)
            except:
                # If we cannot get the voltage, we set a dummy value to 0
                node_voltage = 0
                current_node = node(node_ID, node_voltage)
        else:
            # We do NOT have a grid ID, so we cannot get voltage. We set dummy value to 0
            node_voltage = 0
            current_node = node(node_ID, node_voltage)

        # If we already have data for the start node, we add the power to the existing node instead of creating a new one.
        foundNode = False
        for n in nodes:
            if current_node.node_id == n.node_id:
                n.power_to_child.extend([(int(df.loc[num, START_NODE_INDEX]), float(get_power(num, df).replace(',', '.')))])
                foundNode = True
                break

        if not foundNode:
            current_node.power_to_child.append((int(df.loc[num, START_NODE_INDEX]), float(get_power(num, df).replace(',', '.'))))
            nodes.append(current_node)



def initialize_missing_nodes(nodes, df):
    # This is a wrapper function for the recursive function get_missing_voltage.
    # All code before the get_voltage function can be placed in main, but this improves code readability and understandability.
    # Connector nodes have no measured voltage
    # Therefore, we can calculate the nodes average voltage from its children.
    # This function identifies the missing nodes and calculates their voltage based on their children. We use a recursive function to calculate the voltage for each node, starting from the leaf nodes and working our way up to the root nodes. This way, we can ensure that we have the correct voltage values for all nodes in the tree structure.
    
    # Collect all unique node IDs from the data
    start_nodes = set(df[START_NODE_INDEX])
    end_nodes = set(df[END_NODE_INDEX])
    all_ids = start_nodes | end_nodes
    
    # Existing node IDs
    existing_ids = set(n.node_id for n in nodes)
    missing_ids = all_ids - existing_ids
    
    # Create missing nodes with initial voltage 'NaN'
    for mid in missing_ids:
        new_node = node(mid, 'NaN')
        nodes.append(new_node)
    
    # Create a dictionary for quick access
    nodes_dict = {n.node_id: n for n in nodes}
    
    # Recursive function to compute voltage
    visited = set()
    
    def get_missing_voltage(nid, visited):
        # This is the recursive function
        # It checks if the node has a voltage value. 
        # If it does, it returns that value. 
        # If not, it finds the parents of the node and calculates the average voltage from the parents. 
        # This is done recursively until we reach a node with a voltage value or a leaf node.
        if nid in visited:
            return 0
        visited.add(nid)
        
        node = nodes_dict[nid]
        if node.voltage not in ['NaN', 0]:
            return float(node.voltage)
        
        # Find parents: nodes that connect to this one
        parents = []
        for other_id, other in nodes_dict.items():
            if any(conn[0] == nid for conn in other.power_to_child):
                parents.append(other_id)
        
        if not parents:
            node.voltage = 0
            return 0
        
        parent_volts = [get_missing_voltage(p, visited) for p in parents]
        avg = sum(parent_volts) / len(parent_volts) if parent_volts else 0
        node.voltage = avg
        return avg
    
    # Compute voltages for nodes with missing values
    for n in nodes:
        if n.voltage in ['NaN', 0]:
            get_missing_voltage(n.node_id, visited.copy())


def sort_nodes(nodes):
    # Sort nodes by node ID. This is not strictly necessary, but it makes it easier to read the output and debug.
    nodes.sort(key=lambda x: x.node_id)


def write_output(nodes):
    # Save output to txt file for further use
    output = open("output.txt", "w")
    for n in nodes:
        output.write(f"{n.to_string()} \n")
    output.close()



def main():
    # In a tree structure, parent voltages are parallel to its childrens voltages. 
    # Therefore, the voltage of a parent node can be approximated as the average of its childrens voltages.
    
    # Error prevention
    # Check format of given files.
    if not is_csv(VOLTAGE_FILE):
        print("Error: Voltage file is not in CSV format.")
        sys.exit(1)

    if not is_csv(NODE_DATA_FILE):
        print("Error: Node data file is not in CSV format.")
        sys.exit(1)

    df = initialize_reader()
    nodes = []

    initialize_nodes(df, nodes)
    initialize_missing_nodes(nodes, df)

    sort_nodes(nodes)  
    write_output(nodes)


if __name__ == '__main__':
    main()