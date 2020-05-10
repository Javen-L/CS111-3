# NAME: Anh Mac
# EMAIL: anhmvc@gmail.com
# UID: 905111606

default:
	gcc -Wall -Wextra -lz -o lab1b-client lab1b-client.c
	gcc -Wall -Wextra -lz -o lab1b-server lab1b-server.c

lab1b-client: lab1b-client.c
	gcc -Wall -Wextra -lz -o lab1b-client lab1b-client.c

lab1b-server: lab1b-server.c
	gcc -Wall -Wextra -lz -o lab1b-server lab1b-server.c

clean:
	rm -f lab1b-client lab1b-server *.tar.gz *.txt

dist:
	tar -czvf lab1b-905111606.tar.gz lab1b-server.c lab1b-client.c Makefile README
