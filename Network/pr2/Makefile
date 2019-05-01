CC=gcc
SERVER_SRC=server.c
CLIENT_SRC=client.c
CFLAGS=-O3

server: $(SERVER_SRC)
	$(CC) -o $@ $^

client: $(CLIENT_SRC)
	$(CC) -o $@ $^

all: server client

clean:
	rm -f server client
	rm -f out.txt
