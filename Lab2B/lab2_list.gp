#! /usr/bin/gnuplot
#
# purpose:
#  generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv

# general plot parameters
set terminal png
set datafile separator ","

# lab2b_1.png
set title "List-1: Throughput of synchronization methods"
set xlabel "Number of Threads"
set xrange [0.75:24]
set logscale x 2
set ylabel "Throughput (operations per second)"
set output 'lab2b_1.png'

# grep out only tests with 1000 iterations, protected with spin locks and mutex, non-yield results
plot \
     "< grep -e 'list-none-s,[0-9]*,1000,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'spin-lock' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-m,[0-9]*,1000,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'mutex' with linespoints lc rgb 'green' 

# lab2b_2.png
set title "List-2: Wait-for-lock time for Mutex-protected Threads"
set xlabel "Number of Threads"
set xrange [0.75:]
set logscale x 2
set ylabel "Time per operations (ns)"
set logscale y 10
set output 'lab2b_2.png'

# grep out only tests with 1000 iterations, protected with mutexes, non-yield results
plot \
     "< grep 'list-none-m,[0-9]*,1000,' lab2b_list.csv" using ($2):($8) \
     title 'wait-for-lock time' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,' lab2b_list.csv" using ($2):($7) \
     title 'time-per-op' with linespoints lc rgb 'green' 

# lab2b_3.png
set title "List-3: Protected Iterations that run without failure"
set xlabel "Threads"
set xrange [0.75:]
set logscale x 2
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'

# grep out successful tests with 4 lists, yielded results
plot \
     "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
     title 'Unprotected' with points lc rgb 'red', \
     "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
     title 'Mutex' with points lc rgb 'green', \
     "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
     title 'Spin-lock' with points lc rgb 'blue'

# lab2b_4.png
set title "List-4: Aggregated Throughput for Mutexes-protected Lists"
set xlabel "Number of Threads"
set xrange [0.75:]
set logscale x 2
set ylabel "Throughput (operations per second)"
set logscale y 10
set output 'lab2b_4.png'

# grep out tests protected by mutex, 1000 iterations, non-yield results
plot \
     "< grep 'list-none-m,[0-9]*,[0-9]*,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=1' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=4' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,[0-9]*,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=8' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,[0-9]*,[0-9]*,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=16' with linespoints lc rgb 'orange' 

# lab2b_5.png
set title "List-5: Aggregated Throughput for Spin Locks-protected Lists"
set xlabel "Number of Threads"
set xrange [0.75:]
set logscale x 2
set ylabel "Throughput (operations per second)"
set logscale y 10
set output 'lab2b_5.png'

# grep out tests protected by spin locks, 1000 iterations, non-yield results
plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=1' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=4' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=8' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'lists=16' with linespoints lc rgb 'orange' 
