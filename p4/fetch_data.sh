#!/usr/bin/bash

rm output_data2/times.txt
touch output_data2/times.txt


echo "Starting simulations..."
counter=1
for solution_type in randomwalk
do
    for flow_size in 1M 10M 100M
    do
        echo "Starting simulation $counter"
        counter=$(($counter+1))
        echo "$solution_type $flow_size" >> output_data2/times.txt
        \time --format "%e" -a -o output_data2/times.txt sudo -E python3 network.py $solution_type $flow_size 
    done
done

