import sys
import os
import subprocess
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

        self.init_translation_dicts()

        self.format_topo_for_gofor()
        self.dags = get_dag.get_all_segment_lists(base_name_topo_gofor)
        self.ingress_routers = [x for x in self.topo.get_p4switches() if self.topo.nodes[x].get("host_prefix") is not None]
        self.initialize_segment_lists()

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
            self.mininet_to_gofor[name] = name[1:]
            self.gofor_to_mininet[name[1:]] = name
            # failsafes, not clean but i don't have much time
            self.mininet_to_gofor[name[1:]] = name[1:]
            self.gofor_to_mininet[name] = name
        

    def initialize_tables(self):
        # Grouping of flows (unused as of now)
        for ir in self.ingress_routers:
            host_prefix = self.topo.nodes[ir]["host_prefix"]
            self.controllers[ir].table_add("ipv4_lpm", "set_sr_group", [host_prefix], [base_flow_group, "0"])

    # Only for the ingress routers ( = connected to a prefix in this example)
    def initialize_segment_lists(self):
        for source_ir in self.ingress_routers:
            for dest_ir in self.ingress_routers:
                if (source_ir == dest_ir):
                    continue
                self.initialize_segment_list(source_ir, dest_ir)

    def initialize_segment_list(self, src, dst):
        (gofor_src, gofor_dst) = (self.mininet_to_gofor[src], self.mininet_to_gofor[dst])
        for delay in self.dags[(gofor_src, gofor_dst)].keys():
            for i, seg_list in enumerate(self.dags[(gofor_src, gofor_dst)][delay]["seg_lists"]):
                segment_list_arg = []
                for segment in seg_list:
                    if(segment[-1]["node_adj"] == "Adj"):
                        # do special magic cool stuff (hashing)
                        segment_list_arg.append(str(segment[1])) 
                    else:
                        segment_list_arg.append(str(segment[1]))
                self.controllers[src].table_add(
                    "FEC_tbl", 
                    f"sr_ingress_{len(seg_list)}_hop",
                    [base_flow_group, str(i)],
                    segment_list_arg)

    # TODO add forwarding


if __name__ == "__main__":
    controller = RoutingController()
    controller.initialize_tables()
