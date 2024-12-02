# Variables
CC = gcc
CFLAGS = -Wall

# Objetivos
all: servidor cliente mensajes

servidor: servidor.c
	$(CC) $(CFLAGS) -o servidor servidor.c

cliente: cliente.c
	$(CC) $(CFLAGS) -o cliente cliente.c

mensajes:
	touch mensajes.txt

clean:
	rm -f servidor cliente client_pipe_* server_pipe mensajes.txt
