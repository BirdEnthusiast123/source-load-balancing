from p4utils.mininetlib.network_API import NetworkAPI
import controller_random_walk as ControllerRW
import sys

net = NetworkAPI()
# Network general options
net.setLogLevel('info')

# Network definition
net.addP4Switch('s1')
net.addP4Switch('s2', host_prefix="10.2.0.0/16")
net.addP4Switch('s3')
net.addP4Switch('s4')
net.addP4Switch('s5')
net.addP4Switch('s6')
net.addP4Switch('s7')
net.addP4Switch('s8')
net.addP4Switch('s9', host_prefix="10.9.0.0/16")
net.setP4SourceAll('distribution.p4')

net.addHost('h11')
net.addHost('h12')
net.addHost('h13')
net.addHost('h20')

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

net.addLink("s9", "h11")
net.addLink("s9", "h12")
net.addLink("s9", "h13")
net.addLink("s2", "h20")

# Address assignment strategy
net.l3()

# Nodes general options
net.enableLogAll()
net.disableCli()
net.startNetwork()

# Populate routing tables 
controller = ControllerRW.RoutingController()
controller.init_routing()

# Fetch stats
(h11, h12, h13) = (net.net.get("h11"), net.net.get("h12"), net.net.get("h13"))
host_nodes = [h11, h12, h13]
server_node = net.net.get("h20")

server_node.sendCmd("iperf -s")

host_threads = []
for host_node in host_nodes:
    # host_node.sendCmd("python3 stats.py")
    host_node.sendCmd("iperf -c 10.2.20.2 -n 10M")

for host_node in host_nodes:
    test = host_node.waitOutput()
    print(host_node, test)

print(sys.argv)
