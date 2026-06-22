import random

# This script can be used to generate synthetic data for testing cable parameter estimation using regression

# Amount of samples to generate for slack and load
T = 40
# Amount of load nodes
loads = 3

slack_mean_re = 1.0
slack_std_re = 0.005
slack_mean_im = 0
slack_std_im = 0

load_mean_re = 0.01
load_std_re = 0.009
load_mean_im = 0.002
load_std_im = 0.000005


# Generate T slack voltages
# and T voltages and power consumptions for each load node
# Values can then be copy pasted into C++
def generate_slack_load():
    slack_voltages = []
    load_powers = []

    for i in range(loads):
        load_powers.append([])

    for t in range(T):
        re = abs(random.gauss(slack_mean_re, slack_std_re))
        im = random.gauss(slack_mean_im, slack_std_im)
        slack_voltages.append([re, im])

        for i in range(loads):
            load_re = abs(random.gauss(load_mean_re, load_std_re))
            load_im = random.gauss(load_mean_im, load_std_im)
            load_powers[i].append([load_re, load_im])

    slack_str = "{"
    for u in slack_voltages:
        slack_str += f"{'{'}{u[0]:6}, {u[1]:6}{'}'}, "
    slack_str = slack_str.strip(", ") + "}"

    load_str = "{"
    for load in load_powers:
        load_str += "\n{"
        for s in load:
            load_str += f"{'{'}{s[0]:6}, {s[1]:6}{'}'}, "
        load_str = load_str.strip(", ") + "},"
    load_str = load_str.strip(", ") + "\n}"

    print(slack_str)
    print("=====")
    print(load_str)


# After solving a network using generated slack and load data, print out load voltages for each sample
# and pass them into this function to turn them into easily copy-pasteable values
def translate_load_voltages(load_str: str):
    tok = load_str.split("\n")

    load_voltages = []
    for i in range(loads):
        load_voltages.append([])

    load = 0
    for u in tok:
        u = u.strip("()")
        load_voltages[load].append(u)
        load = (load + 1) % loads

    print("{")
    for x in load_voltages:
        print("{{" + "}, {".join(x).strip(", {") + "}}")
    print("}")


generate_slack_load()
#translate_load_voltages()