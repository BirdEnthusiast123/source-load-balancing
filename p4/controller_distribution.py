from p4utils.utils.helper import load_topo
from p4utils.utils.sswitch_thrift_API import SimpleSwitchThriftAPI
import nnpy
import struct
import pandas
from scapy.all import Ether, sniff, Packet, BitField, raw
# requires sim.py which requires a configures config.default.ini file
import get_dag

base_name_topo_gofor = "topo-gofor.txt"
base_flow_group = "0"

class CpuHeader(Packet):
    name = 'CpuPacket'
    fields_desc = [BitField('time_spent_in_pipeline',0,64), BitField('ingress',0,64), BitField('egress',0,64)]

class RoutingController(object):

    def __init__(self):
        self.digest = [] # TODO to remove
        self.cpu_array = [] # TODO to remove
        self.topo = load_topo('topology.json')
        self.cpu_port =  self.topo.get_cpu_port_index("s9") # TODO REMOVE
        self.init_translation_dicts() # gofor code does not support chars, p4-utils does not support the absence of "s" or "h" in the name
        self.format_topo_for_gofor()

        self.controllers = {}
        self.connect_to_switches()
        self.reset_states()
        self.controllers["s9"].mirroring_add(100, self.cpu_port) # TODO REMOVE

        self.dags = get_dag.get_all_segment_lists(base_name_topo_gofor)
        self.ingress_routers = [x for x in self.topo.get_p4switches() if self.topo.nodes[x].get("host_prefix") is not None]

        self.flowlet_leniency_factor = 1.1 # 10% of the RTT 

    def init_routing(self):
        self.init_registers()
        self.initialize_tables()
        self.initialize_segment_lists()
        self.init_sr_forward_node()
        self.init_sr_forward_adj()

    def format_topo_for_gofor(self):
        with open(base_name_topo_gofor, "w") as outfile_topo_gofor:
            for edge in self.topo.edges(data=True):
                if(edge[-1].get("delay") is None):
                    continue
                (src, dst) = (self.mininet_to_gofor[edge[0]], self.mininet_to_gofor[edge[1]])
                (delay, igp) = (edge[-1]["delay"], edge[-1]["igp_cost"])
                outfile_topo_gofor.write("{} {} {} {}\n".format(src, dst, delay, igp))

    def reset_states(self):
        [controller.reset_state() for controller in list(self.controllers.values())]

    def connect_to_switches(self):
        for p4switch in self.topo.get_p4switches():
            thrift_port = self.topo.get_thrift_port(p4switch)
            self.controllers[p4switch] = SimpleSwitchThriftAPI(thrift_port)

    def init_translation_dicts(self):
        (self.mininet_to_gofor, self.gofor_to_mininet) = ({}, {})
        for name in self.topo.nodes.keys():
            self.mininet_to_gofor[name] = name[1:] # "sN" -> "N"
            self.gofor_to_mininet[name[1:]] = name # "N" -> "sN"
            # failsafes, not clean but i don't have much time
            self.mininet_to_gofor[name[1:]] = name[1:]
            self.gofor_to_mininet[name] = name
        self.topo_to_p4 = self.mininet_to_gofor
        
    def init_registers(self):
        for p4switch in self.topo.get_p4switches():
            self.controllers[p4switch].register_write("sr_id_register", 0, self.mininet_to_gofor[p4switch])

    def initialize_tables(self):
        # Grouping of flows (unused as of now)
        for ir in self.ingress_routers:
            # Init forwarding to hosts directly connected to router
            for host in self.topo.get_hosts_connected_to(ir):
                link = self.topo.edges[(ir, host)]
                ip_addr = self.topo.get_host_ip(host)
                if(ir == link["node1"]):
                    self.controllers[ir].table_add("ipv4_lpm", "ipv4_forward", [ip_addr + "/32"], [link["addr2"], str(link["port1"])])
                else:
                    self.controllers[ir].table_add("ipv4_lpm", "ipv4_forward", [ip_addr + "/32"], [link["addr1"], str(link["port2"])])

            # Init forwarding to distant hosts
            for ir2 in self.ingress_routers:
                if(ir == ir2):
                    continue
                host_prefix = self.topo.nodes[ir2]["host_prefix"]
                self.controllers[ir].table_add("ipv4_lpm", "set_sr_group", [host_prefix], [self.mininet_to_gofor[ir2], "0"])

    # Only for the ingress routers ( = connected to a prefix in this example)
    def initialize_segment_lists(self):
        for source_ir in self.ingress_routers:
            for dest_ir in self.ingress_routers:
                if (source_ir == dest_ir):
                    continue
                self.initialize_segment_list(source_ir, dest_ir)

    # do special magic cool stuff (hashing/masking)
    # over 20 bits : 1 bit of label mask
    # 7 for the source, 7 for the destination
    # 5 for the link index (hash of the weights of the link)
    def get_link_id(self, src, dst):
        (src, dst) = (int(src), int(dst))
        (mn_src, mn_dst) = (self.gofor_to_mininet[str(src)], self.gofor_to_mininet[str(dst)])
        link_igp = self.topo.edges[(mn_src, mn_dst)]["igp_cost"]
        link_delay = self.topo.edges[(mn_src, mn_dst)]["delay"]
        link_hash = hash(str(link_igp) + str(link_delay)) % 32
        bit_segment = (1 << 18) + (link_hash << 14) + (src << 7) + dst
        return bit_segment


    def initialize_segment_list(self, src, dst):
        (gofor_src, gofor_dst) = (self.mininet_to_gofor[src], self.mininet_to_gofor[dst])
        seg_list_index = 0
        for delay in self.dags[(gofor_src, gofor_dst)].keys():
            for seg_list in self.dags[(gofor_src, gofor_dst)][delay]["seg_lists"]:
                segment_list_arg = [str(int(int(delay) * 2 * 1000 * self.flowlet_leniency_factor))]
                for segment in seg_list:
                    if(segment[-1]["node_adj"] == "Adj"):
                        (seg_src, seg_dst) = (segment[0], segment[1])
                        adj_segment = self.get_link_id(seg_src, seg_dst)
                        segment_list_arg.insert(0, str(adj_segment))
                    else:
                        segment_list_arg.insert(0, str(segment[1]))

                self.controllers[src].table_add("FEC_tbl", f"sr_ingress_{len(seg_list)}_hop", [gofor_dst, str(seg_list_index)], segment_list_arg)
                seg_list_index += 1

        if(self.topo.nodes[dst].get("host_prefix") is not None):
            self.controllers[src].table_modify_match("ipv4_lpm", "set_sr_group", [self.topo.nodes[dst]["host_prefix"]], [gofor_dst, str(seg_list_index)])

    def add_sr_forward_entry(self, src, dest_segment, next_hop):
        if(src == self.topo.edges[(src, next_hop)]["node1"]):
            mac_addr = self.topo.edges[(src, next_hop)]["addr2"]
            port = self.topo.edges[(src, next_hop)]["port1"]
            self.controllers[src]. table_add("sr_tbl", "sr_forward", [str(dest_segment)], [mac_addr, str(port)])
        else:
            mac_addr = self.topo.edges[(src, next_hop)]["addr1"]
            port = self.topo.edges[(src, next_hop)]["port2"]
            self.controllers[src]. table_add("sr_tbl", "sr_forward", [str(dest_segment)], [mac_addr, str(port)])

    def add_sr_ecmp_forward_entry(self, src, dest_segment, next_hop, index):
        if(src == self.topo.edges[(src, next_hop)]["node1"]):
            mac_addr = self.topo.edges[(src, next_hop)]["addr2"]
            port = self.topo.edges[(src, next_hop)]["port1"]
            self.controllers[src]. table_add("ecmp_group_to_nhop", "sr_forward", [str(dest_segment), str(index)], [mac_addr, str(port)])
        else:
            mac_addr = self.topo.edges[(src, next_hop)]["addr1"]
            port = self.topo.edges[(src, next_hop)]["port2"]
            self.controllers[src]. table_add("ecmp_group_to_nhop", "sr_forward", [str(dest_segment), str(index)], [mac_addr, str(port)])


    ###
    ### If there are multiple paths, ecmp among them
    ###
    def init_sr_forward_node(self):
        switches = [x for x in self.topo.get_p4switches()]
        all_pairs = get_dag.np.array(get_dag.np.meshgrid(switches, switches)).T.reshape(-1,2)
        for (src, dst) in all_pairs:
            if(src == dst):
                continue

            (src_id, dst_id) = (self.topo_to_p4[src], self.topo_to_p4[dst])

            paths_generator = get_dag.nx.all_shortest_paths(self.topo, src, dst, "igp_cost")
            paths = [x for x in paths_generator] # generator to array

            if len(paths) == 1:
                next_hop = paths[0][1]
                self.add_sr_forward_entry(src, dst_id, next_hop)
            elif len(paths) > 1:
                self.controllers[src].table_add("sr_tbl", "set_ecmp_group", [dst_id], [dst_id, str(len(paths))])
                for index, path in enumerate(paths):
                    next_hop = path[1]
                    self.add_sr_ecmp_forward_entry(src, dst_id, next_hop, index)
    

    def init_sr_forward_adj(self):
        switches = [x for x in self.topo.get_p4switches()]
        for switch in switches:
            src_id = self.topo_to_p4[switch]
            for neighbor in self.topo.neighbors(switch):
                if (neighbor not in switches):
                    continue
                dest_segment = self.get_link_id(src_id, self.mininet_to_gofor[neighbor])
                self.add_sr_forward_entry(switch, dest_segment, neighbor)


# digests which contain the time passed in the data plane pipeline
    def unpack_digest(self, msg, num_samples):
        starting_index = 32
        for sample in range(num_samples):
            time_passed_in_pipeline = struct.unpack(">Q", msg[starting_index:starting_index+8])
            starting_index +=8
            self.digest.append(time_passed_in_pipeline[0])

    def recv_msg_digest(self, msg):
        topic, device_id, ctx_id, list_id, buffer_id, num = struct.unpack("<iQiiQi", msg[:32])
        self.unpack_digest(msg, num)
        #Acknowledge digest
        self.controllers["s9"].client.bm_learning_ack_buffer(ctx_id, list_id, buffer_id)

    def run_digest_loop(self):
        sub = nnpy.Socket(nnpy.AF_SP, nnpy.SUB)
        notifications_socket = self.controllers["s9"].client.bm_mgmt_get_info().notifications_socket
        sub.connect(notifications_socket)
        sub.setsockopt(nnpy.SUB, nnpy.SUB_SUBSCRIBE, '')
        i = 0
        while i < 5000:
            msg = sub.recv()
            self.recv_msg_digest(msg)
            i += 1

    def recv_msg_cpu(self, pkt):
        packet = Ether(raw(pkt))
        if packet.type == 0x1234:
            cpu_header = CpuHeader(bytes(packet.load))
            print(cpu_header.ingress, cpu_header.egress)
            self.cpu_array.append(cpu_header.time_spent_in_pipeline)
            if(len(self.cpu_array) % 100 == 0):
                print(pandas.DataFrame(self.cpu_array).describe())


    def run_cpu_port_loop(self):
        cpu_port_intf = str(self.topo.get_cpu_port_intf("s9").replace("eth0", "eth1"))
        sniff(iface=cpu_port_intf, prn=self.recv_msg_cpu)

if __name__ == "__main__":
    controller = RoutingController()
    controller.init_routing()
    controller.run_cpu_port_loop()

    times_spent_in_pipeline = [x - controller.digest[i-1] for (i, x) in enumerate(controller.digest[1:])]
    times_spent_in_pipeline = [x for x in times_spent_in_pipeline if abs(x) < 4000]
    print(pandas.DataFrame(times_spent_in_pipeline).describe())
        