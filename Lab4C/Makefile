# NAME: Anh Mac
# EMAIL: anhmvc@gmail.com
# UID: 905111606

default:
	gcc -Wall -Wextra -lm -lmraa lab4c_tcp.c -o lab4c_tcp
	gcc -Wall -Wextra -lm -lmraa -lssl -lcrypto lab4c_tls.c -o lab4c_tls

clean:
	rm -f lab4c_tcp lab4c_tls *.tar.gz

dist: default
	tar -czvf lab4c-905111606.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README 