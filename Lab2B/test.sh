#!/bin/bash

# NAME: Anh Mac
# EMAIL: anhmvc@gmail.com
# UID: 905111606

numThreads=(1 2 4 8 12 16 24)
numThreads2=(1 4 8 12 16)
numThreads3=(1 2 4 8 12)
numIterations=(1 2 4 8 16)
numIterations2=(10 20 40 80)
numLists=(1 4 8 16)
syncopts=(s m)

# 1000 iterations, mutex tests
for i in "${numThreads[@]}"; do
    ./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2b_list.csv
done

# 1000 iterations, spin-lock tests
for i in "${numThreads[@]}"; do
    ./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2b_list.csv
done

# see how many iterations it takes to reliably fail with no synchronization
for i in "${numThreads2[@]}"; do
    for j in "${numIterations[@]}"; do
        ./lab2_list --threads=$i --iterations=$j --yield=id --lists=4 >> lab2b_list.csv
    done
done

for i in "${numThreads2[@]}"; do
    for j in "${numIterations2[@]}"; do
        for k in "${syncopts[@]}"; do
            ./lab2_list --threads=$i --iterations=$j --yield=id --sync=$k --lists=4 >> lab2b_list.csv
        done
    done
done

for i in "${numThreads3[@]}"; do
    for j in "${numLists[@]}"; do
        for k in "${syncopts[@]}"; do
            ./lab2_list --threads=$i --iterations=1000 --lists=$j --sync=$k >> lab2b_list.csv
        done
    done
done


