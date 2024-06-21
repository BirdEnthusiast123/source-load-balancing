import sim
import sys
import subprocess
import pandas
import os
import shutil


base_use_case = "DCLCSR-pareto-loose-all"
base_output_folder = "segment_lists"
base_output_filename = "segment_list.txt"

def get_distances(distances_file):
    df = pandas.read_csv(distances_file, delim_whitespace=True)
    df = df.drop(df[df.SRC == "SRC"].index) # rermove duplicates of keys throughout the file
    return df

def parametrize_get_list_call(cmd, src, dest, dist, output_folder):
    cmd += ["--src", src]
    cmd += ["--print-solution", "dest={},m1={}".format(dest, dist)] #uncomment for debug
    cmd += ["--print-segment-list", "dest={},m1={}".format(dest, dist)]
    output_file_with_source = str(src) + "-" + base_output_filename
    output_path = "/".join([output_folder, output_file_with_source])
    return (cmd, output_path) 

def init_paths(topology):
    distances_file = "/".join(["../data/model-real", topology.split('/')[-1], base_use_case + "-DIST"])
    output_folder = "/".join(["../data/model-real", topology.split('/')[-1], base_output_folder])
    return (distances_file, output_folder)


###
### In order to retrieve segment lists we need : 
### - To compute the DAG and the "distances" separating every pair of nodes
### - The distances as parameter to compute the segment lists
### - The source, the destination and the additional --print-segment-list option (which is a wrapper around the -print-solution which shows whole trees)
###
def main():
    if(len(sys.argv) != 2):
        print("Usage : {} <topology file>".format(sys.argv[0])) 

    # populate the distances file
    get_distances_cmd = sim.runOnFileCmd(sys.argv[1], use_cases=base_use_case, force=True)

    (distances_file, output_folder) = init_paths(sys.argv[1])
    distances_df = get_distances(distances_file)
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
    else:
        shutil.rmtree(output_folder)
        os.makedirs(output_folder)
    
    get_segment_list_template_cmd = [x for x in get_distances_cmd if(not x.startswith("--all-nodes"))]

    for index, row in distances_df.iterrows():
        (src, dst, dist) = (row["SRC"], row["DST"], row["DELAY"])
        if(dst == src):
            continue

        get_segment_list_cmd, output_path = parametrize_get_list_call(get_segment_list_template_cmd.copy(), src, dst, dist, output_folder)
        # print("Running: {}".format(' '.join(get_segment_list_cmd))) #uncomment for debug

        with open(output_path, "a") as outfile:
            ret = subprocess.run(get_segment_list_cmd, stdout=outfile, check=True)
            if ret.returncode != 0:
                print("get_segments_list: Invalid return code: {}".format(ret.returncode))
                print(ret.stdout)
        outfile.close()
    
if __name__ == "__main__":
    main()