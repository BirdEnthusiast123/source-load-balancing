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



figure, axis = plt.subplots(3)

temp_df1 = df_dict[("naif", "3", "1M")]["df"].copy()
temp_df1["distribution"] = df_dict[("distribution", "3", "1M", "1.1")]["df"]["distribution"]
temp_df1.distribution = temp_df1.distribution.astype(float)
temp_df1["randomwalk"] = df_dict[("randomwalk", "3", "1M", "1.1")]["df"]["randomwalk"]
temp_df1.randomwalk = temp_df1.randomwalk.astype(float)
temp_df1["perfect"] = df_dict[("perfect", "3", "1M")]["df"]["perfect"]
temp_df1.perfect = temp_df1.perfect.astype(float)

temp_df2 = df_dict[("naif", "3", "10M")]["df"].copy()
temp_df2["distribution"] = df_dict[("distribution", "3", "10M", "1.1")]["df"]["distribution"]
temp_df2.distribution = temp_df2.distribution.astype(float)
temp_df2["randomwalk"] = df_dict[("randomwalk", "3", "10M", "1.1")]["df"]["randomwalk"]
temp_df2.randomwalk = temp_df2.randomwalk.astype(float)
temp_df2["perfect"] = df_dict[("perfect", "3", "10M")]["df"]["perfect"]
temp_df2.perfect = temp_df2.perfect.astype(float)

temp_df3 = df_dict[("naif", "3", "100M")]["df"].copy()
temp_df3["distribution"] = df_dict[("distribution", "3", "100M", "1.1")]["df"]["distribution"]
temp_df3.distribution = temp_df3.distribution.astype(float)
temp_df3["randomwalk"] = df_dict[("randomwalk", "3", "100M", "1.1")]["df"]["randomwalk"]
temp_df3.randomwalk = temp_df3.randomwalk.astype(float)
temp_df3["perfect"] = df_dict[("perfect", "3", "100M")]["df"]["perfect"]
temp_df3.perfect = temp_df3.perfect.astype(float)



temp_df1.plot.density(ax=axis[0])
temp_df2.plot.density(ax=axis[1])
temp_df3.plot.density(ax=axis[2])
axis[0].set_title("FCT pour 3 flux à 1M") 
axis[0].set_xlabel("FCT")
axis[0].set_ylabel("Densité")
axis[0].set_yticks([])
axis[1].set_title("FCT pour 3 flux à 10M") 
axis[1].set_xlabel("FCT")
axis[1].set_ylabel("Densité")
axis[1].set_yticks([])
axis[2].set_title("FCT pour 3 flux à 100M") 
axis[2].set_xlabel("FCT")
axis[2].set_ylabel("Densité")
axis[2].set_yticks([])

plt.tight_layout()
plt.savefig("test.svg")