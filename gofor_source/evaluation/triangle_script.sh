#!/usr/bin/bash

rm -rf ../data/model-real/topo-example-tri-mail.txt

python3 sim.py runonfile ../code/topo-example-tri-mail.txt --use-cases DCLCSR-pareto-loose-all

../code/SR/samcraDCLC --output ../data/model-real/topo-example-tri-mail.txt/DCLCSR-pareto-loose-all-TIME \
 --results ../data/model-real/topo-example-tri-mail.txt/DCLCSR-pareto-loose-all-DIST                     \
 --bi-dir --id --src 0 --topo --file ../data/model-real/topo-example-tri-mail.txt/topo.txt               \
 --sr-bin --sr-conv ../data/model-real/topo-example-tri-mail.txt/sr.bin --encoding loose --mode pareto   \
 --retrieve all --print-solution dest=$1,m1=$2 --print-segment-list dest=$1,m1=$2                        \
 >> ../data/model-real/topo-example-tri-mail.txt/segments_lists.txt
