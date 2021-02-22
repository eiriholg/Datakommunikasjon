BIN = client server 
DEBUG = -g 
CFLAGS = -std=gnu11 -c
MEMCHECK = valgrind --track-origins=yes --leak-check=full

all: $(BIN)

client: client.o pgmread.o send_packet.o filefunctions.o
	gcc $(DEBUG) client.o filefunctions.o pgmread.o send_packet.o -o client

server: server.o filefunctions.o pgmread.o
	gcc $(DEBUG) server.o filefunctions.o pgmread.o -o server

client.o: client.c 
	gcc $(CFLAGS) client.c

server.o: server.c
	gcc $(CFLAGS) server.c  

filefunctions.o: filefunctions.c
	gcc $(CFLAGS) filefunctions.c

unpack.o: unpack.c pgmread.h
	gcc $(CFLAGS) unpack.c

pgmread.o: pgmread.c pgmread.h
	gcc $(CFLAGS) pgmread.c 

send_packet.o: send_packet.c send_packet.h
	gcc $(CFLAGS) send_packet.c

run_client: 
	$(MEMCHECK) ./client 127.0.0.1 1234 list_of_filenames.txt 5
run_server:
	$(MEMCHECK) ./server 1234 big_set outputfile.txt
clean:
	rm *.o 
	rm -f $(BIN) 
