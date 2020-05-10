#!/bin/bash

# NAME: Anh Mac
# EMAIL: anhmvc@gmail.com
# UID: 905111606

numThreads=(1 2 4 8 12)
numIterations=(1 10 20 40 80 100 1000 10000 100000)
syncOpts=(m s c)

# add-none tests
for i in "${numIterations[@]}"; do
    for j in "${numThreads[@]}"; do
	    ./lab2_add --iterations=$i --threads=$j >> lab2_add.csv
    done
done
echo "Completed add-none tests"

# add-none-yield tests
for i in "${numIterations[@]}"; do
    for j in "${numThreads[@]}"; do
	    ./lab2_add --iterations=$i --threads=$j --yield >> lab2_add.csv
    done
done
echo "Completed add-yield-none tests"


# synchronizations, no yield tests
for i in "${syncOpts[@]}"; do
    for j in "${numIterations[@]}"; do
	    for k in "${numThreads[@]}"; do
            ./lab2_add --iterations=$j --threads=$k --sync=$i >> lab2_add.csv
        done
    done
    echo "Completed add-$i tests" 
done 

# synchronizations, yield tests
for i in "${syncOpts[@]}"; do
    for j in "${numIterations[@]}"; do
	    for k in "${numThreads[@]}"; do
	        ./lab2_add --iterations=$j --threads=$k --sync=$i --yield >> lab2_add.csv
	    done
    done
    echo "Completed add-yield-$i tests"
done

