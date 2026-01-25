CC = gcc
CFLAGS = -Wall

all: deliver/deliver server/server

deliver/deliver: deliver/deliver.c
	$(CC) $(CFLAGS) deliver/deliver.c -o deliver/deliver

server/server: server/server.c
	$(CC) $(CFLAGS) server/server.c -o server/server

clean:
	rm -f deliver/deliver server/server