# NAME: Anh Mac
# EMAIL: anhmvc@gmail.com
# UID: 905111606

default:
	gcc -Wall -Wextra -g -lpthread -lprofiler lab2_list.c SortedList.c -o lab2_list

tests: default
	rm -f *.csv
	chmod u+x test.sh
	-./test.sh 2>/dev/null

profile:
	rm -f profile.out
	LD_PRELOAD=/u/cs/ugrad/mac/Desktop/CS111/Lab2B/lib/libprofiler.so.0  
	CPUPROFILE=./myfile.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text lab2_list myfile.gperf > profile.out
	pprof --list=thread_routine lab2_list myfile.gperf >> profile.out

graphs: lab2b_list.csv
	rm -f *.png
	chmod u+x lab2_list.gp
	./lab2_list.gp

clean:
	rm -f lab2_list lab2b-905111606.tar.gz

dist: tests graphs clean
	tar -czvf lab2b-905111606.tar.gz lab2_list.c SortedList.h SortedList.c lab2b_list.csv lab2_list.gp Makefile test.sh profile.out README *.png



