#!/usr/bin/bash

for filename in /home/florenthardy/gofor_source/data/model-real/topo-example-DAG.txt/dags/*.dot; do
	echo "Running : "
	echo "dot -Tpng $filename -o $filename.png"
	dot -Tpng "$filename" -o "$filename.png"
done
