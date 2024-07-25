from p4utils.utils.helper import load_topo
from p4utils.utils.sswitch_thrift_API import SimpleSwitchThriftAPI
import nnpy
import struct
import pandas
from scapy.all import Ether, sniff, Packet, BitField, raw
# requires sim.py which requires a filled in config.default.ini file
import get_dag

base_name_topo_gofor = "topo-gofor.txt"
base_flow_group = "0"

class CpuHeader(Packet):
    name = 'CpuPacket'
    fields_desc = [BitField('time_spent_in_pipeline',0,64), BitField('ingress',0,64), BitField('egress',0,64)]

class RoutingController(object):

    def __init__(self):
        self.topo = load_topo('topology.json')
        self.init_translation_dicts() # gofor code does not support chars, p4-utils does not support the absence of "s" or "h" in the name
        self.format_topo_for_gofor()

        self.controllers = {}
        self.connect_to_switches()
        self.reset_states()
        
        self.dags = get_dag.get_all_segment_lists(base_name_topo_gofor)
        self.ingress_routers = [x for x in self.topo.get_p4switches() if self.topo.nodes[x].get("host_prefix") is not None]

        self.flowlet_leniency_factor = 1.1 # 10% of the RTT 

    def init_routing(self):
        self.init_registers()
        self.initialize_tables()
        self.init_sr_forward_node()
        self.init_sr_forward_adj()
        self.initialize_all_dags()

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
        # Grouping of flows (unused as of now, only serves as a destination marker not as a real discriminator of flow according to their needs)
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
                (gofor_ir, gofor_ir2) = (self.mininet_to_gofor[ir], self.mininet_to_gofor[ir2])
                nb_of_first_seg_hops = 0 
                for delay in self.dags[(gofor_ir, gofor_ir2)].keys():
                    dag = self.dags[(gofor_ir, gofor_ir2)][delay]["dag"]
                    nb_of_first_seg_hops = nb_of_first_seg_hops + len(dag.out_edges([int(gofor_ir)]))
                self.controllers[ir].table_add("ipv4_lpm", "set_sr_group", [host_prefix], [self.mininet_to_gofor[ir2], str(nb_of_first_seg_hops)])

    # Only for the ingress routers ( = connected to a prefix in this example)
    def initialize_all_dags(self):
        for source_ir in self.ingress_routers:
            for dest_ir in self.ingress_routers:
                if (source_ir == dest_ir):
                    continue
                self.initialize_dags(source_ir, dest_ir)

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

    def initialize_dags(self, src, dst):
        (gofor_src, gofor_dst) = (self.mininet_to_gofor[src], self.mininet_to_gofor[dst])
        index_dict = {}
        for delay in self.dags[(gofor_src, gofor_dst)].keys():
            dag = self.dags[(gofor_src, gofor_dst)][delay]["dag"]
            # since multiple paths can travserse a node in a meta-dag, we need to keep track of how many hops there are per path and not in total
            self.traversal_and_initialize_dag(dag, src, gofor_dst, self.mininet_to_gofor[src], index_dict)
    
    # necessary if current node has multiple paths passing through
    def get_nb_of_seg_hops(self, dag: get_dag.nx.MultiDiGraph, hop_dst, path_weight):
        nb = 0
        # in a single meta-dag there can be multiple paths passing by a single node, we use weights to diffenetiate them
        for out_edge in dag.out_edges(hop_dst, data=True):
            nb = (nb + 1) if (path_weight == (out_edge[-1]["sum_weight"] - out_edge[-1]["weight"])) else nb
        return nb

    def traversal_and_initialize_dag(self, dag: get_dag.nx.MultiDiGraph, src, dst, current_segment, index_dict, depth=1, current_sum_of_costs=0):
        if(int(current_segment) == int(dst)):
            return

        out_edges = dag.out_edges([int(self.mininet_to_gofor[str(current_segment)])], data=True)
        for edge in out_edges:
            (edge_src, edge_dst, edge_attributes) = (edge[0], edge[1], edge[-1])

            if((current_sum_of_costs + edge_attributes["weight"]) != edge_attributes["sum_weight"]):
                continue

            # used for the next iteration of the random walk so the program knows from which paths to choose
            future_path_weight = edge_attributes["sum_weight"]

            segment = self.get_link_id(edge_src, edge_dst) if (edge_attributes["node_adj"] == "Adj") else edge_dst
            index_dict[(edge_src, current_sum_of_costs)] = (index_dict[(edge_src, current_sum_of_costs)] + 1) if (index_dict.get((edge_src, current_sum_of_costs)) is not None) else 0
            if(int(edge_dst) != int(dst)):
                nb_of_seg_hops = self.get_nb_of_seg_hops(dag, edge_dst, future_path_weight)
                self.controllers[src].table_add("segment_hop_" + str(depth), 
                                                "set_segment_hop", 
                                                [str(dst), str(current_segment), str(current_sum_of_costs), str(index_dict[(edge_src, current_sum_of_costs)])], 
                                                [str(segment), str(future_path_weight), str(nb_of_seg_hops)])
            else:
                rtt_in_ms = (current_sum_of_costs + edge_attributes["weight"]) * 1000 * 2
                rtt_with_leniency = int(rtt_in_ms * self.flowlet_leniency_factor)
                self.controllers[src].table_add("segment_hop_" + str(depth), 
                                                "set_last_segment_hop", 
                                                [str(dst), str(current_segment), str(current_sum_of_costs), str(index_dict[(edge_src, current_sum_of_costs)])], 
                                                [str(segment), str(rtt_with_leniency)])
                
            self.traversal_and_initialize_dag(dag, src, dst, edge_dst, index_dict, depth + 1, current_sum_of_costs + edge_attributes["weight"])

    def add_sr_forward_entry(self, src, dest_segment, next_hop):
        if(src == self.topo.edges[(src, next_hop)]["node1"]):
            mac_addr = self.topo.edges[(src, next_hop)]["addr2"]
            port = self.topo.edges[(src, next_hop)]["port1"]
            self.controllers[src].table_add("sr_tbl", "sr_forward", [str(dest_segment)], [mac_addr, str(port)])
        else:
            mac_addr = self.topo.edges[(src, next_hop)]["addr1"]
            port = self.topo.edges[(src, next_hop)]["port2"]
            self.controllers[src].table_add("sr_tbl", "sr_forward", [str(dest_segment)], [mac_addr, str(port)])

    def add_sr_ecmp_forward_entry(self, src, dest_segment, next_hop, index):
        if(src == self.topo.edges[(src, next_hop)]["node1"]):
            mac_addr = self.topo.edges[(src, next_hop)]["addr2"]
            port = self.topo.edges[(src, next_hop)]["port1"]
            self.controllers[src].table_add("ecmp_group_to_nhop", "sr_forward", [str(dest_segment), str(index)], [mac_addr, str(port)])
        else:
            mac_addr = self.topo.edges[(src, next_hop)]["addr1"]
            port = self.topo.edges[(src, next_hop)]["port2"]
            self.controllers[src].table_add("ecmp_group_to_nhop", "sr_forward", [str(dest_segment), str(index)], [mac_addr, str(port)])


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

    def clean_workspace(self):
        if get_dag.os.path.exists("/".join(["gofor_source/data/model-real", base_name_topo_gofor, ""])):
            get_dag.shutil.rmtree("/".join(["gofor_source/data/model-real", base_name_topo_gofor, ""]))

if __name__ == "__main__":
    controller = RoutingController()
    controller.init_routing()
    # controller.run_cpu_port_loop()

    # times_spent_in_pipeline = [x - controller.digest[i-1] for (i, x) in enumerate(controller.digest[1:])]
    # times_spent_in_pipeline = [x for x in times_spent_in_pipeline if abs(x) < 4000]
    # print(pandas.DataFrame(times_spent_in_pipeline).describe())