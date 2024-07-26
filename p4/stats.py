import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os
import seaborn as sns

directory_name = "output_data2"
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



# figure, axis = plt.subplots(3)

# temp_df1 = df_dict[("naif", "1M")]["df"].copy()
# temp_df1["distribution"] = df_dict[("distribution", "1M")]["df"]["distribution"]
# temp_df1.distribution = temp_df1.distribution.astype(float)
# temp_df1["randomwalk"] = df_dict[("randomwalk", "1M")]["df"]["randomwalk"]
# temp_df1.randomwalk = temp_df1.randomwalk.astype(float)
# temp_df1["naif2"] = df_dict[("naif2", "1M")]["df"]["naif2"]
# temp_df1.naif2 = temp_df1.naif2.astype(float)
# # temp_df1["perfect"] = df_dict[("perfect", "1M")]["df"]["perfect"]
# # temp_df1.perfect = temp_df1.perfect.astype(float)

# temp_df2 = df_dict[("naif", "10M")]["df"].copy()
# temp_df2["distribution"] = df_dict[("distribution", "10M")]["df"]["distribution"]
# temp_df2.distribution = temp_df2.distribution.astype(float)
# temp_df2["randomwalk"] = df_dict[("randomwalk", "10M")]["df"]["randomwalk"]
# temp_df2.randomwalk = temp_df2.randomwalk.astype(float)
# temp_df2["naif2"] = df_dict[("naif2", "10M")]["df"]["naif2"]
# temp_df2.naif2 = temp_df2.naif2.astype(float)
# # temp_df2["perfect"] = df_dict[("perfect", "10M")]["df"]["perfect"]
# # temp_df2.perfect = temp_df2.perfect.astype(float)

# temp_df3 = df_dict[("naif", "100M")]["df"].copy()
# temp_df3["distribution"] = df_dict[("distribution", "100M")]["df"]["distribution"]
# temp_df3.distribution = temp_df3.distribution.astype(float)
# temp_df3["randomwalk"] = df_dict[("randomwalk", "100M")]["df"]["randomwalk"]
# temp_df3.randomwalk = temp_df3.randomwalk.astype(float)
# temp_df3["naif2"] = df_dict[("naif2", "100M")]["df"]["naif2"]
# temp_df3.naif2 = temp_df3.naif2.astype(float)
# # temp_df3["perfect"] = df_dict[("perfect", "100M")]["df"]["perfect"]
# # temp_df3.perfect = temp_df3.perfect.astype(float)


# test2 = sns.kdeplot(temp_df1, ax=axis[0])
# # test2.savefig("test2.png")
# # sns.regplot(data=temp_df1, ax=axis[0])
# # temp_df1.plot.density(ax=axis[0])
# temp_df2.plot.density(ax=axis[1])
# temp_df3.plot.density(ax=axis[2])
# axis[0].set_title("FCT pour 3 flux à 1M") 
# axis[0].set_xlabel("FCT")
# axis[0].set_ylabel("Densité")
# axis[0].set_yticks([])
# axis[1].set_title("FCT pour 3 flux à 10M") 
# axis[1].set_xlabel("FCT")
# axis[1].set_ylabel("Densité")
# axis[1].set_yticks([])
# axis[2].set_title("FCT pour 3 flux à 100M") 
# axis[2].set_xlabel("FCT")
# axis[2].set_ylabel("Densité")
# axis[2].set_yticks([])




temp_df3 = df_dict[("naif", "100M")]["df"].copy()
temp_df3["distribution"] = df_dict[("distribution", "100M")]["df"]["distribution"]
temp_df3.distribution = temp_df3.distribution.astype(float)
temp_df3["randomwalk"] = df_dict[("randomwalk", "100M")]["df"]["randomwalk"]
temp_df3.randomwalk = temp_df3.randomwalk.astype(float)
temp_df3["naif2"] = df_dict[("naif2", "100M")]["df"]["naif2"]
temp_df3.naif2 = temp_df3.naif2.astype(float)
# temp_df3["perfect"] = df_dict[("perfect", "100M")]["df"]["perfect"]
# temp_df3.perfect = temp_df3.perfect.astype(float)


# sns.displot(data=temp_df3, kind="kde")
sns.boxplot(data=temp_df3)


# plt.vlines(df_dict[("perfect", "100M")]["df"]["perfect"].mean(), ymin=0, ymax=0.012, colors=["black"], linestyles="dotted", label="perfect")
plt.legend()
plt.tight_layout()
plt.savefig("test.svg")