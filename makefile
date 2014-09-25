all:server16 client16
server16: server16.o 
	gcc -Wall -o server16 server16.o 
client16: client16.o 
	gcc -Wall -o client16 client16.o
clean:
	rm -f *.o
	rm -f server16 client16