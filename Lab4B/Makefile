# NAME: Anh Mac
# EMAIL: anhmvc@gmail.com
# UID: 905111606

default:
	gcc -Wall -Wextra -lm -lmraa lab4b.c -o lab4b

check: default
	chmod u+x smokeCheck.sh
	./smokeCheck.sh

clean:
	rm -f lab4b lab4b-905111606.tar.gz

dist: default
	tar -czvf lab4b-905111606.tar.gz lab4b.c Makefile README smokeCheck.sh