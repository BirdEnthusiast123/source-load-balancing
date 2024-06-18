import sim
import sys
import subprocess
import pandas
import os
import shutil
import numpy as np
import networkx as nx
import pylab as plt
import itertools as it
import re


base_use_case = "DCLCSR-pareto-loose-all"
base_output_folder = "dags"
base_output_filename = "dag.txt"

def get_distances(distances_file):
    df = pandas.read_csv(distances_file, delim_whitespace=True)
    df = df.drop(df[df.SRC == "SRC"].index) # rermove duplicates of keys throughout the file
    return df

def parametrize_get_dag_call(cmd, src, dest, dist, output_folder):
    cmd += ["--src", src]
    # cmd += ["--print-solution", "dest={},m1={}".format(dest, dist)] #uncomment for debug
    cmd += ["--print-dag", "dest={},m1={}".format(dest, dist)]
    # output_file_with_source = str(src) + "_" + str(dest) + "_" + base_output_filename
    output_file_with_source = f"{src}_{dest}_{dist}_{base_output_filename}"
    output_path = "/".join([output_folder, output_file_with_source])
    return (cmd, output_path) 

def init_paths(topology):
    distances_file = "/".join(["../data/model-real",topology.split('/')[-1], base_use_case + "-DIST"])
    output_folder = "/".join(["../data/model-real", topology.split('/')[-1], base_output_folder])
    return (distances_file, output_folder)

def init_files(distances_df, output_folder):
    (srcs, dsts) = (np.unique(distances_df["SRC"]), np.unique(distances_df["DST"]))
    src_dest_enum = np.array(np.meshgrid(srcs, dsts)).T.reshape(-1,2)

    for src, dst in src_dest_enum:
        if(src == dst):
            continue
        for _, row in distances_df.iterrows():
            if(row["SRC"] == src and row["DST"] == dst):
                all_info_filename = f"{src}_{dst}_{row['DELAY']}_{base_output_filename}"
                with open("/".join([output_folder, all_info_filename]), "w") as f:
                    f.write("src dst node/adj delay\n")
                    f.close()


def write_dags(distances_df, output_folder, template_cmd):
    all_dags = {}
    for _, row in distances_df.iterrows():
        (src, dst, dist) = (row["SRC"], row["DST"], row["DELAY"])

        if(dst == src):
            continue

        if(all_dags.get((src, dst)) is None):
            all_dags[(src, dst)] = {}
        if(all_dags[(src, dst)].get(dist) is None):
            all_dags[(src, dst)][dist] = {}
    
        get_dag_cmd, output_path = parametrize_get_dag_call(template_cmd.copy(), src, dst, dist, output_folder)
        all_dags[(src, dst)][dist]["path"] = output_path

        # print("Running: {}".format(' '.join(get_dag_cmd))) #uncomment for debug
        with open(output_path, "a") as outfile:
            ret = subprocess.run(get_dag_cmd, stdout=outfile, check=True)
            if ret.returncode != 0:
                print("get_segments_list: Invalid return code: {}".format(ret.returncode))
                print(ret.stdout)
            outfile.close()
    
    return all_dags

def parse_dag_file(dag_file):
    df = pandas.read_csv(dag_file, delim_whitespace=True)
    df = df.drop_duplicates()
    return df

def init_dag(dag_df):
    dag = nx.MultiDiGraph()
    for _, row in dag_df.iterrows():
        src, dst, node_adj, cost = row["src"], row["dst"], row["node/adj"], row["delay"]
        dag.add_edge(src, dst, weight=cost, label=node_adj)
    return dag

def init_all_dags(all_dags):
    for (src, dst), dists in all_dags.items():
        for dist in dists.keys():
            dag_df = parse_dag_file(all_dags[(src, dst)][dist]["path"])
            dag = init_dag(dag_df)
            all_dags[(src, dst)][dist]["dag"] = dag
    return all_dags

# def traversal(dag: nx.MultiDiGraph, src, dst, total_cost):
#     current_node = src
#     path_stack = []
#     segment_lists = []

#     edges_visit = {edge : False for edge in dag.edges}
#     print(edges_visit)
#     while(not all(edge_visit for edge_visit in edges_visit.values()) or path_stack[-1] != dst):
#         # TODO add to end of while condition or we will skip the last path
#         # if(current_node == dst and sum_of_costs == total_cost):
#         #     segment_lists.append(path_stack.copy())
#         #     path_stack.pop()
#         #     current_node = path_stack[-1]
#         #     continue
#         # print(dag.nodes)
#         for neigh_edge in dag.out_edges(current_node):
#             print("hehe", current_node)
#             print(neigh_edge)
#             edges_visit[neigh_edge] = True
#             path_stack[-1].append(neigh_edge)
#             current_node = neigh_edge[1]

def travsersal(dag: nx.MultiDiGraph, src, dst, path, total_cost, sum_of_costs=0, prev_cost=0):
    print(total_cost, src, dst, sum_of_costs)
    if(src == dst and total_cost == sum_of_costs):
        print("path", path)
        return
    for edge in dag.out_edges(src, keys=True, data=True):
        print(edge)
        travsersal(dag, int(edge[1]), dst, path + [edge], total_cost, sum_of_costs + (edge[-1]["weight"] - prev_cost), edge[-1]["weight"])



###
### In order to retrieve the DAGs we need : 
### - To compute the "distances" separating every pair of nodes
### - The distances as parameter to compute the DAGs
### - The source, the destination and the additional --print-dag option (which is a wrapper around the -print-solution which shows whole trees)
###
def main():
    if(len(sys.argv) != 2 and len(sys.argv) != 3):
        print("Usage : {} <topology file>".format(sys.argv[0])) 
        return
    
    if(len(sys.argv) == 3 and sys.argv[2] == "clean"):
        shutil.rmtree("/".join(["../data/model-real", sys.argv[1].split('/')[-1], base_output_folder]))
        return

    # populate the distances file
    get_distances_cmd = sim.runOnFileCmd(sys.argv[1], use_cases=base_use_case, force=True)

    (distances_file, output_folder) = init_paths(sys.argv[1])
    distances_df = get_distances(distances_file)
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
    else:
        shutil.rmtree(output_folder)
        os.makedirs(output_folder)
    
    get_dag_template_cmd = [x for x in get_distances_cmd if(not x.startswith("--all-nodes"))]

    init_files(distances_df, output_folder)
    all_output_dag_files = write_dags(distances_df, output_folder, get_dag_template_cmd)

    all_dags = init_all_dags(all_output_dag_files)
    # for i in all_dags[("0", "4")]:
    #     print(i, all_dags[("0", "4")][i].items())

    #     nx.nx_pydot.write_dot(all_dags[("0", "4")][i]["dag"], all_dags[("0", "4")][i]["path"] + ".dot")
    #     subprocess.run(["sed", "-i", "-E", "s/label=([^,]+),/label=\"\\1\",/g", all_dags[("0", "4")][i]["path"] + ".dot"])

    # traversal(all_dags[("0", "4")]["6"]["dag"], 0, 4, 6)
    print(all_dags[("0", "4")]["6"]["dag"].nodes)
    travsersal(all_dags[("0", "4")]["6"]["dag"], 0, 4, [], 6)




if __name__ == "__main__":
    main()
