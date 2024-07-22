import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os

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

for key, df in df_dict.items():
    print(key)
    pass    


temp_df = df_dict[("randomwalk", "5", "100M", "1.1")]["df"].copy()
temp_df["distribution"] = df_dict[("distribution", "5", "100M", "1.1")]["df"]["distribution"]
temp_df.distribution = temp_df.distribution.astype(float)

print(temp_df)

ax = temp_df.plot.density()
plt.savefig("test.png")