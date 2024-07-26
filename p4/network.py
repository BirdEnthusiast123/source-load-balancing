import sys
if(len(sys.argv) != 3):
    print(f"Usage : sudo -E {sys.argv[0]} distribution.p4/randomwalk.p4 <flow size ex: 100M, 15G>")
    exit()
(solution_version, flow_size) = (x for x in sys.argv[1:])

import os
from p4utils.mininetlib.network_API import NetworkAPI

if solution_version == "distribution":
    import controller_distribution as Controller
elif solution_version == "randomwalk":
    import controller_random_walk as Controller

NB_OF_SIMS = 10
output_folder = "output_data2"

net = NetworkAPI()

# Network definition
net.addP4Switch("s1", host_prefix="10.1.0.0/16")
net.addP4Switch("s2", host_prefix="10.2.0.0/16")
net.addP4Switch("s3", host_prefix="10.3.0.0/16")
net.addP4Switch("s4", host_prefix="10.4.0.0/16")
net.addP4Switch("s5")
net.addP4Switch("s6", host_prefix="10.6.0.0/16")
net.setP4SourceAll(solution_version + ".p4")

net.addHost("h11")
net.addHost("h14")
net.addHost("h16")
net.addHost("h23")
net.addHost("h26")
net.addHost("h22")

hosts = {"h11": "h23", "h14": "h26", "h16": "h22"}
server_hosts = {"h23" : "10.3.23.2", "h26": "10.6.26.2", "h22": "10.2.22.2"}

net.addLink("s1", "s2", igp_cost=1, delay=0.2)
net.addLink("s1", "s4", igp_cost=1, delay=0.2)
net.addLink("s1", "s5", igp_cost=1, delay=0.1)
net.addLink("s2", "s3", igp_cost=1, delay=0.2)
net.addLink("s3", "s4", igp_cost=1, delay=0.2)
net.addLink("s3", "s6", igp_cost=3, delay=0.1)
net.addLink("s4", "s5", igp_cost=2, delay=0.1)
net.addLink("s5", "s6", igp_cost=1, delay=0.2)

net.addLink("s1", "h11")
net.addLink("s2", "h22")
net.addLink("s3", "h23")
net.addLink("s4", "h14")
net.addLink("s6", "h16")
net.addLink("s6", "h26")

# Address assignment strategy
net.l3()

# Nodes general options
# net.enableLogAll()
net.disableCli()
net.startNetwork()

# Initialize controller
controller = Controller.RoutingController()
controller.flowlet_leniency_factor = 0.0

# Fetch stats
mininet_hosts = {net.net.get(key): net.net.get(val) for key, val in hosts.items()}
server_nodes = {net.net.get(key): val for key, val in server_hosts.items()}
output = {x: [] for x in mininet_hosts}

# Simulate
for server_node in server_nodes:
    server_node.sendCmd("iperf --server --daemon")

for i in range(NB_OF_SIMS):
    print(i)
    controller.reset_states()
    controller.init_routing()

    for mininet_host, val in mininet_hosts.items():
        mininet_host.sendCmd(f"\\time -f \"%e\" iperf -c {server_nodes[mininet_hosts[mininet_host]]} -n {flow_size} > /dev/null")

    for mininet_host in mininet_hosts.keys():
        output[mininet_host].append(mininet_host.waitOutput().strip())

output_file_name = f"naif2_{flow_size}.csv"

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

