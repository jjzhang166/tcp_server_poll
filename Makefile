CC=gcc
CFLAGS= -g -Wall

all: tcp_server 

tcp_server: tcp_server.c

clean:
	rm -rf tcp_server
