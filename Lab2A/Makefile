# NAME: Anh Mac
# EMAIL: anhmvc@gmail.com
# UID: 905111606

build: lab2_add.c lab2_list.c SortedList.c SortedList.h
	gcc -Wall -Wextra -pthread lab2_add.c -o lab2_add
	gcc -Wall -Wextra -pthread lab2_list.c SortedList.c -o lab2_list

tests: build
	rm -f lab2_add.csv lab2_list.csv
	chmod +x test_add.sh
	chmod +x test_list.sh
	./test_add.sh 
	-./test_list.sh 2>/dev/null

graphs: lab2_add.csv lab2_list.csv
	chmod u+x lab2_add.gp lab2_list.gp
	./lab2_add.gp 2> /dev/null
	./lab2_list.gp 2> /dev/null

clean:
	rm -f lab2_add lab2_list *.tar.gz *.gch

dist: tests graphs clean
	tar -czvf lab2a-905111606.tar.gz lab2_add.c lab2_list.c SortedList.h SortedList.c lab2_add.csv lab2_list.csv Makefile test_list.sh test_add.sh lab2_add.gp lab2_list.gp README *.png
