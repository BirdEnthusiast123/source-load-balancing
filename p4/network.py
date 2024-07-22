import sys
if(len(sys.argv) != 5):
    print(f"Usage : sudo -E {sys.argv[0]} distribution.p4/randomwalk.p4 <flow size ex: 100M, 15G> <nb of iperf hosts> <flowlet leniency factor>")
    exit()
(solution_version, flow_size, nb_of_hosts, flowlet_leniency_factor) = (x for x in sys.argv[1:])

import os
from p4utils.mininetlib.network_API import NetworkAPI

if solution_version == "distribution":
    import controller_distribution as Controller
elif solution_version == "randomwalk":
    import controller_random_walk as Controller

NB_OF_SIMS = 50
output_folder = "output_data"

net = NetworkAPI()

# Network definition
net.addP4Switch("s1")
net.addP4Switch("s2", host_prefix="10.2.0.0/16")
net.addP4Switch("s3")
net.addP4Switch("s4")
net.addP4Switch("s5")
net.addP4Switch("s6")
net.addP4Switch("s7")
net.addP4Switch("s8")
net.addP4Switch("s9", host_prefix="10.9.0.0/16")
net.setP4SourceAll(solution_version + ".p4")

hosts = [] # cannot use net.hosts() because we need to exclude the "server" host h20
for i in range(int(nb_of_hosts)):
    hosts.append(net.addHost("h1" + str(i + 1)))

net.addHost("h20") # iperf server 

net.addLink("s1", "s2", igp_cost=2, delay=1.0)
net.addLink("s1", "s3", igp_cost=1, delay=0.1)
net.addLink("s1", "s4", igp_cost=1, delay=0.1)
net.addLink("s3", "s2", igp_cost=2, delay=0.8)
net.addLink("s4", "s5", igp_cost=1, delay=0.2)
net.addLink("s4", "s6", igp_cost=1, delay=0.3)
net.addLink("s4", "s7", igp_cost=1, delay=0.4)
net.addLink("s4", "s8", igp_cost=1, delay=0.5)
net.addLink("s4", "s2", igp_cost=1, delay=0.9)
net.addLink("s5", "s2", igp_cost=1, delay=0.6)
net.addLink("s6", "s2", igp_cost=1, delay=0.5)
net.addLink("s7", "s2", igp_cost=1, delay=0.4)
net.addLink("s8", "s2", igp_cost=1, delay=0.3)
net.addLink("s1", "s9", igp_cost=1, delay=0.1)

for host in hosts:
    net.addLink("s9", host)
net.addLink("s2", "h20")

# Address assignment strategy
net.l3()

# Nodes general options
# net.enableLogAll()
net.disableCli()
net.startNetwork()

# Initialize controller
controller = Controller.RoutingController()
controller.flowlet_leniency_factor = float(flowlet_leniency_factor)

# Fetch stats
mininet_hosts = [net.net.get(x) for x in hosts]
server_node = net.net.get("h20")
output = {x: [] for x in mininet_hosts}

# Simulate
server_node.sendCmd("iperf --server --daemon")

for i in range(NB_OF_SIMS):
    controller.reset_states()
    controller.init_routing()

    for mininet_host in mininet_hosts:
        mininet_host.sendCmd(f"\\time -f \"%e\" iperf -c 10.2.20.2 -n {flow_size} > /dev/null")

    for mininet_host in mininet_hosts:
        output[mininet_host].append(mininet_host.waitOutput().strip())

output_file_name = f"{solution_version}_{nb_of_hosts}_{flow_size}_{flowlet_leniency_factor}.csv"

if not os.path.exists(output_folder):
    os.makedirs(output_folder)

with open("/".join([output_folder, output_file_name]), "w") as f:
    keys_string = " ".join([key.name for key in output.keys()])
    f.write(keys_string + "\n")

    for i in range(NB_OF_SIMS):
        values_str = ""
        for host in output:
            values_str += str(output[host][i]) + " "
        f.write(values_str + "\n")

