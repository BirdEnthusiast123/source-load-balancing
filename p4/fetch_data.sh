#!/usr/bin/bash

rm output_data/times.txt
touch output_data/times.txt


echo "Starting simulations..."
counter=1
for solution_type in distribution random_walk
do
    for flowlet_leniency_factor in 1.1 1.2 1.3 1.4 1.5
    do 
        for flow_size in 1M 10M 100M
        do
            for nb_of_hosts in 3 5 7
            do
                echo "Starting simulation $counter of 120..."
                counter=$(($counter+1))
                echo "$solution_type $flow_size $nb_of_hosts $flowlet_leniency_factor" >> output_data/times.txt
                \time --format "%e" -a -o output_data/times.txt sudo -E python3 network.py $solution_type $flow_size $nb_of_hosts $flowlet_leniency_factor
            done
        done
    done
done

