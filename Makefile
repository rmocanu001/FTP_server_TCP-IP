CC = gcc
CFLAGS = -Wall -Wextra -g


all: client server

client: client.o

server: server.o

client.o: client.c

server.o: server.c

