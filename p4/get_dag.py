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


base_use_case = "DCLCSR-pareto-loose-all"
base_output_folder = "dags"
base_output_filename = "dag.txt"
base_output_filename_weights = "weights_dg.txt"

def get_distances(distances_file):
    df = pandas.read_csv(distances_file, delim_whitespace=True)
    df = df.drop(df[df.SRC == "SRC"].index) # rermove duplicates of keys throughout the file
    df = df.drop_duplicates()
    return df

def parametrize_get_dag_call(cmd, src, dest, dist, output_folder):
    cmd += ["--src", src]
    # cmd += ["--print-solution", "dest={},m1={}".format(dest, dist)] #uncomment for debug
    cmd += ["--print-dag", "dest={},m1={}".format(dest, dist)]
    output_file_with_source = f"{src}_{dest}_{dist}_{base_output_filename}"
    output_path = "/".join([output_folder, output_file_with_source])
    return (cmd, output_path) 

def parametrize_get_dag_weights_call(cmd, src, dest, dist, output_folder):
    cmd += ["--src", src]
    cmd += ["--print-weights-dag", "dest={},m1={}".format(dest, dist)]
    output_file_with_source = f"{src}_{dest}_{dist}_{base_output_filename_weights}"
    output_path = "/".join([output_folder, output_file_with_source])
    return (cmd, output_path)

def init_paths(topology_path):
    distances_file = "/".join([sim.DataPath, "model-real", topology_path.split('/')[-1], base_use_case + "-DIST"])
    output_folder = "/".join([sim.DataPath, "model-real", topology_path.split('/')[-1], base_output_folder])
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
                    f.close()
                all_info_filename_weights = f"{src}_{dst}_{row['DELAY']}_{base_output_filename_weights}"
                with open("/".join([output_folder, all_info_filename_weights]), "w") as f:
                    f.close()


def write_dags(distances_df, output_folder, template_cmd):
    all_dags = {}
    for _, row in distances_df.iterrows():
        (src, dst, delay) = (row["SRC"], row["DST"], row["DELAY"])

        if(dst == src):
            continue

        if(all_dags.get((src, dst)) is None):
            all_dags[(src, dst)] = {}
        if(all_dags[(src, dst)].get(delay) is None):
            all_dags[(src, dst)][delay] = {}
        else:
            continue

        get_dag_cmd, output_path = parametrize_get_dag_call(template_cmd.copy(), src, dst, delay, output_folder)
        all_dags[(src, dst)][delay]["path"] = output_path
        # print("Running: {}".format(' '.join(get_dag_cmd))) #uncomment for debug
        with open(output_path, "a") as outfile_dag:
            ret = subprocess.run(get_dag_cmd, stdout=outfile_dag)
            if ret.returncode != 0:
                print("get_segments_list: Invalid return code: {}".format(ret.returncode))
                print(ret.stdout)
        outfile_dag.close()

        get_dag_weights_cmd, output_weights_path = parametrize_get_dag_weights_call(template_cmd.copy(), src, dst, delay, output_folder)
        all_dags[(src, dst)][delay]["weights_path"] = output_weights_path
        # print("Running: {}".format(' '.join(get_dag_weights_cmd))) #uncomment for debug
        with open(output_weights_path, "a") as outfile_dag_weights:
            ret = subprocess.run(get_dag_weights_cmd, stdout=outfile_dag_weights)
            if ret.returncode != 0:
                print("get_segments_list: Invalid return code: {}".format(ret.returncode))
                print(ret.stdout)
        outfile_dag_weights.close()        
    
    return all_dags

def parse_dag_file(dag_file):
    df = pandas.read_csv(dag_file, delim_whitespace=True)
    df = df.drop_duplicates()
    return df

def init_dag(dag_df: pandas.DataFrame):
    dag = nx.MultiDiGraph()
    for _, row in dag_df.iterrows():
        src, dst, node_adj, delay = row["src"], row["dst"], row["node/adj"], row["delay"]
        dag.add_edge(src, dst, sum_weight=delay, node_adj=node_adj)
    return dag

def init_dag_weights(weights_dag_df, dag: nx.MultiDiGraph):
    for _, row in weights_dag_df.iterrows():
        src, dst, delay = row["src"], row["dst"], row["delay"]
        dag.edges[(src, dst, 0)]["weight"] = delay
        # there can be duplicates of src, dst, and delay edges if one is Node and the other Adj 
        if(dag.edges.get((src, dst, 1)) is not None):
            dag.edges[(src, dst, 1)]["weight"] = delay
    return dag

def init_all_dags(all_dags):
    for (src, dst), delays in all_dags.items():
        for delay in delays.keys():
            dag_df = parse_dag_file(all_dags[(src, dst)][delay]["path"])
            dag = init_dag(dag_df)
            dag_weights_df = parse_dag_file(all_dags[(src, dst)][delay]["weights_path"])
            dag = init_dag_weights(dag_weights_df, dag)
            all_dags[(src, dst)][delay]["dag"] = dag
    return all_dags

###
### In order to discriminate the meta-DAG multiple paths per node :
### - One execution gives us the sum of the costs of the path for each step
### - The other gives us the weight of each edge
### - We check if the sum of individual edges corresponds to the returned (by the gofor exe) sum of costs
### - If it doesn't the path isn't the one we were looking for
###
def traversal_get_seg_lists(dag: nx.MultiDiGraph, src, dst, total_cost, path=[], sum_of_costs=0, paths=[]):
    if(int(src) == int(dst) and int(total_cost) == sum_of_costs):
        return paths.append(path)
    for edge in dag.out_edges([int(src)], keys=True, data=True):
        if(sum_of_costs + int(edge[-1]["weight"]) == edge[-1]["sum_weight"]):
            traversal_get_seg_lists(dag, edge[1], dst, total_cost, path + [edge], sum_of_costs + edge[-1]["weight"], paths)
    return paths

def get_all_seg_lists(all_dags):
    for (src, dst) in all_dags.keys():
        for delay in all_dags[(src, dst)].keys():
            dag = all_dags[(src, dst)][delay]["dag"]
            all_dags[(src, dst)][delay]["seg_lists"] = traversal_get_seg_lists(dag, src, dst, delay, paths=[])
    return all_dags
    
### Basicallly an importable version of what main() does
def get_all_segment_lists(topo_path: str):
    get_distances_cmd = sim.runOnFileCmd(topo_path, use_cases=base_use_case, force=True)

    (distances_file, output_folder) = init_paths(topo_path)
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
    all_dags = get_all_seg_lists(all_dags)

    return all_dags

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
    all_dags = get_all_seg_lists(all_dags)


if __name__ == "__main__":
    main()
