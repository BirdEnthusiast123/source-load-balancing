import sys
import os
from p4utils.utils.helper import load_topo
from p4utils.utils.sswitch_thrift_API import SimpleSwitchThriftAPI
# requires sim.py which requires a configures config.default.ini file
import get_dag

class RoutingController(object):

    def __init__(self, subnets=0):

        self.topo = load_topo('topology.json')
        print(self.topo.nodes)
        print(self.topo.edges)
        self.controllers = {}

        self.connect_to_switches()
        self.reset_states()
        self.set_table_defaults()

        self.format_topo_for_gofor()

    def format_topo_for_gofor(self):
        for edge in self.topo.edges(data=True, keys=True):
            print(edge[0], edge[1], edge[-1]["delay"], edge[-1]["igp_cost"])

    def reset_states(self):
        [controller.reset_state() for controller in list(self.controllers.values())]

    def connect_to_switches(self):
        for p4switch in self.topo.get_p4switches():
            thrift_port = self.topo.get_thrift_port(p4switch)
            self.controllers[p4switch] = SimpleSwitchThriftAPI(thrift_port)

    def set_table_defaults(self):
        for controller in list(self.controllers.values()):
            controller.table_set_default("ipv4_lpm", "drop", [])

    def initialize_tables(self, entries=0):

        # Set static rules
        self.controllers["s1"].table_add("ipv4_lpm", "set_nhop_index", ["10.0.1.2/32"], ["1"])
        self.controllers["s1"].table_add("forward", "_forward", ["1"], [self.topo.get_host_mac('h1'), "1"])

        self.controllers["s2"].table_add("ipv4_lpm", "set_nhop_index", ["10.0.1.0/24"], ["1"])
        self.controllers["s2"].table_add("forward", "_forward", ["1"], ["00:00:0a:00:01:02", "1"])
        self.controllers["s2"].table_add("ipv4_lpm", "set_nhop_index", ["10.0.0.0/8"], ["2"])
        self.controllers["s2"].table_add("forward", "_forward", ["2"], ["00:00:0a:00:01:02", "2"])

        self.controllers["s3"].table_add("ipv4_lpm", "set_nhop_index", ["10.0.1.0/24"], ["1"])
        self.controllers["s3"].table_add("forward", "_forward", ["1"], ["00:00:0a:00:01:02", "1"])
        self.controllers["s3"].table_add("ipv4_lpm", "set_nhop_index", ["10.0.0.0/8"], ["2"])
        self.controllers["s3"].table_add("forward", "_forward", ["2"], ["00:00:0a:00:01:02", "2"])

        self.controllers["s4"].table_add("ipv4_lpm", "set_nhop_index", ["10.0.1.0/24"], ["2"])
        self.controllers["s4"].table_add("forward", "_forward", ["2"], ["00:00:0a:00:01:02", "2"])
        self.controllers["s4"].table_add("ipv4_lpm", "set_nhop_index", ["10.0.2.2/32"], ["3"])
        self.controllers["s4"].table_add("forward", "_forward", ["3"], [self.topo.get_host_mac('h2'), "3"])
        self.controllers["s4"].table_add("ipv4_lpm", "set_nhop_index", ["10.250.250.2/32"], ["4"])
        self.controllers["s4"].table_add("forward", "_forward", ["4"], [self.topo.get_host_mac('h3'), "4"])

        # Dynamic entries for s1
        self.controllers["s1"].table_add("forward", "_forward", ["2"], ["00:00:00:02:01:00", "2"])

        for entry in self.destination_subnets:
            self.controllers["s1"].table_add("ipv4_lpm", "set_nhop_index", [entry], ["2"])

    def recover_from_failure(self, out):
        self.controllers["s1"].table_modify_match("forward", "_forward", ["2"], ["00:00:00:03:01:00", str(out)])


if __name__ == "__main__":
    controller = RoutingController()
    controller.initialize_tables()
