#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <pthread.h>
#include <time.h>

#define SERVER_PIPE "server_pipe"
#define MAX_TOPICS 20
#define TOPIC_NAME_LEN 21 // espacio adicional para el caracter nulo
#define MAX_SUBSCRIBERS 10
#define USERNAME_LEN 257 // espacio adicional para el caracter nulo
#define MAX_USERS 10
#define MAX_MESSAGES 100
#define TAM_MSG 301 // espacio adicional para el caracter nulo
