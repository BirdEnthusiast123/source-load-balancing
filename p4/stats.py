import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os

directory_name = "output_data"
directory = os.fsencode(directory_name)

df_dict = {}

for file in os.listdir(directory):
    filename = os.fsdecode(file)
    if(filename.endswith(".csv")):
        filename_no_ext = os.path.splitext(filename)[0]
        characteristrics = tuple([x for x in filename_no_ext.split("_")])

        df = pd.read_csv("/".join([directory_name, filename]), delim_whitespace=True)
        one_dimensional_df = df.melt(value_name=characteristrics[0], var_name="tmp").drop("tmp", axis=1)
        df_dict[characteristrics] = {}
        df_dict[characteristrics]["df"] = one_dimensional_df



figure, axis = plt.subplots(2, 2)

temp_df1 = df_dict[("ecmp", "3", "1M")]["df"].copy()
temp_df1["randomwalk"] = df_dict[("randomwalk", "3", "1M", "1.1")]["df"]["randomwalk"]
temp_df1.randomwalk = temp_df1.randomwalk.astype(float)

temp_df2 = df_dict[("ecmp", "3", "10M")]["df"].copy()
temp_df2["randomwalk"] = df_dict[("randomwalk", "3", "10M", "1.1")]["df"]["randomwalk"]
temp_df2.randomwalk = temp_df2.randomwalk.astype(float)

temp_df3 = df_dict[("ecmp", "3", "10M")]["df"].copy()
temp_df3["randomwalk"] = df_dict[("randomwalk", "3", "10M", "1.1")]["df"]["randomwalk"]
temp_df3.randomwalk = temp_df3.randomwalk.astype(float)

temp_df4 = df_dict[("ecmp", "3", "100M")]["df"].copy()
temp_df4["randomwalk"] = df_dict[("randomwalk", "3", "100M", "1.1")]["df"]["randomwalk"]
temp_df4.randomwalk = temp_df4.randomwalk.astype(float)


temp_df1.plot.density(ax=axis[0, 0])
temp_df2.plot.density(ax=axis[0, 1])
temp_df3.plot.density(ax=axis[1, 0])
temp_df4.plot.density(ax=axis[1, 1])
axis[0, 0].set_title("Title 1") 
axis[0, 1].set_title("Title 2") 
axis[1, 0].set_title("Title 3") 
axis[1, 1].set_title("Title 4") 

plt.tight_layout()
plt.savefig("test.svg")