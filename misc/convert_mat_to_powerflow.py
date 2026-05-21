from scipy.io import loadmat
import sys

"""
    This script converts .mat files to network files that PowerFlow can use.
    How to use:
        From command line run "python convert_mat_to_powerflow.py src dst" where "src" is the filename of the .mat file, 
        and "dst" is the filename to save the network to. 
        "dst" should be a .txt file
"""

src = sys.argv[1]
dst = sys.argv[2]

data = loadmat(src)
nodeCount = data["num_nodes"][0][0] - 1

net = "grid\n" + str(data["S_base"][0][0]) + " " + str(data["U_base"][0][0]) + "\n"
# Write connections between nodes + impedances. Zero-impedances get set to (0.0000001, 0.0000001)
for i in range(nodeCount):
    start = data["cable_start_end_node"][i][0] - 1
    end = data["cable_start_end_node"][i][1] - 1
    Z = str(data["Z_cable"][i][0]).strip("()j")
    net += str(start) + " " + str(end) + " (" + (", ".join(Z.split("+")) if Z != "0" else "0.0000001, 0.0000001") + ")"

    net += "\n"

# Set all leaf nodes to be load nodes, set root node to slack
net += "%\n"
for i in range(nodeCount):
    isLoad = data["busIsLoad"][i][0] == 1
    isSlack = i == 0

    if isSlack:
        net += str(i) + " s\n"
    elif isLoad:
        net += str(i) + " l\n"

# Assume the whole network is one grid => no connections here
net += "%\nconnections\n%"

# Write to dst
with open(dst, "w") as file:
    file.write(net)
