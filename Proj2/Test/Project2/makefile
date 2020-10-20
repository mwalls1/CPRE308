server: server.o
	gcc -o server server.o -lpthread
server.o:
	gcc -c server.c -lpthread
server2: server2.o
	gcc -o server2 server2.o -lpthread
server2.o:
	gcc -c server2.c -lpthread
Bank: Bank.o
	gcc -o Bank Bank.o
Bank.o:
	gcc -c Bank.c
clean: 
	rm Bank *.o
	rm server *.o
	rm server2 *.o

