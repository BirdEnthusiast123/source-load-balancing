from p4utils.utils.helper import load_topo
from p4utils.utils.sswitch_thrift_API import SimpleSwitchThriftAPI
# requires sim.py which requires a configures config.default.ini file
import get_dag

base_name_topo_gofor = "topo-gofor.txt"
base_flow_group = "0"

class RoutingController(object):

    def __init__(self):
        self.topo = load_topo('topology.json')

        self.controllers = {}
        self.connect_to_switches()
        self.reset_states()

        self.init_translation_dicts() # gofor code does not support chars, p4-utils does not support the absence of "s" or "h" in the name

        self.format_topo_for_gofor()
        self.dags = get_dag.get_all_segment_lists(base_name_topo_gofor)
        self.ingress_routers = [x for x in self.topo.get_p4switches() if self.topo.nodes[x].get("host_prefix") is not None]

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
            host_prefix = self.topo.nodes[ir]["host_prefix"]
            host = self.topo.get_hosts_connected_to(ir)[0]
            link = self.topo.edges[(ir, host)]
            if(ir == link["node1"]):
                self.controllers[ir].table_add("ipv4_lpm", "ipv4_forward", [host_prefix], [link["addr2"], str(link["port1"])])
            else:
                self.controllers[ir].table_add("ipv4_lpm", "ipv4_forward", [host_prefix], [link["addr1"], str(link["port2"])])


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
        link_igp = self.topo.edges[self.gofor_to_mininet[str(src)], self.gofor_to_mininet[str(dst)]]["igp_cost"]
        link_delay = self.topo.edges[self.gofor_to_mininet[str(src)], self.gofor_to_mininet[str(dst)]]["delay"]
        link_hash = hash(str(link_igp) + str(link_delay)) % 32
        bit_segment = (1 << 18) + (src << 12) + (dst << 5) + link_hash
        return bit_segment


    def initialize_segment_list(self, src, dst):
        (gofor_src, gofor_dst) = (self.mininet_to_gofor[src], self.mininet_to_gofor[dst])
        seg_list_index = 0
        for delay in self.dags[(gofor_src, gofor_dst)].keys():
            for seg_list in self.dags[(gofor_src, gofor_dst)][delay]["seg_lists"]:
                segment_list_arg = []
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

if __name__ == "__main__":
    controller = RoutingController()
