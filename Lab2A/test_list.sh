#!/bin/bash

# NAME: Anh Mac
# EMAIL: anhmvc@gmail.com
# UID: 905111606

numThreads=(2 4 8 12)
numThreads2=(1 2 4 8 12 16 24)
numIterations=(10 100 1000 10000 20000)
numIterations2=(1 10 100 1000)
numIterations3=(1 2 4 8 16 32)
numIterations4=(1 2 4 8 12 16 24 32)
syncOpts=(s m)
yieldOpts=(i d l id il dl idl)

# single thread tests
for i in "${numIterations[@]}"; do
    ./lab2_list --threads=1 --iterations=$i >> lab2_list.csv
done
echo "Completed single thread tests"

# test parallel threads and iterations
for i in "${numIterations2[@]}"; do
    for j in "${numThreads[@]}"; do
	    ./lab2_list --iterations=$i --threads=$j >> lab2_list.csv
    done
done
echo "Completed parallel threads/iterations tests"

# various combinations of yield options
for i in "${numIterations3[@]}"; do
    for j in "${numThreads[@]}"; do
        for k in "${yieldOpts[@]}"; do
	        ./lab2_list --iterations=$i --threads=$j --yield=$k >> lab2_list.csv
        done
    done
done
echo "Completed yield tests"

# test protected options
for i in "${numIterations4[@]}"; do
    for j in "${numThreads[@]}"; do
        for k in "${yieldOpts[@]}"; do
            for l in "${syncOpts[@]}"; do
	            ./lab2_list --iterations=$i --threads=$j --yield=$k --sync=$l >> lab2_list.csv
            done
        done
    done
done
echo "Completed protected options tests"

# test 1000 iterations, w/o yields, range of # threads, with locks
for i in "${numThreads2[@]}"; do
    for j in "${syncOpts[@]}"; do
        ./lab2_list --iterations=1000 --threads=$i --sync=$j >> lab2_list.csv
    done 
done
echo "Completed 1000 iterations, no yields, range of threads, with locks tests"
echo "All tests completed"
