# Variables
CC = gcc
CFLAGS = -Wall

# Objetivos principales
all: servidor cliente mensajes


# Reglas para generar los binarios
servidor: servidor.o util.h
	$(CC) $(CFLAGS) -o servidor servidor.o

cliente: cliente.o util.h
	$(CC) $(CFLAGS) -o cliente cliente.o

# Reglas para generar archivos .o
servidor.o: servidor.c util.h
	$(CC) $(CFLAGS) -c servidor.c -o servidor.o

cliente.o: cliente.c util.h
	$(CC) $(CFLAGS) -c cliente.c -o cliente.o

# Regla para el archivo de mensajes
mensajes:
	touch mensajes.txt

# Limpiar archivos generados
clean:
	rm -f servidor cliente servidor.o cliente.o client_pipe_* server_pipe mensajes.txt
