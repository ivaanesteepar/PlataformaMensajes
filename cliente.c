#include "util.h"

// Struct de comunicación con el manager
typedef struct {
    char client_pipe[256];
    int command_type;
    char topic[50];
    char username[50];
    pid_t pid;
    int lifetime;
    char message[TAM_MSG];
} Request;

Request msg;

// Función para enviar un comando al servidor
void send_command_to_server(Request *msg) {
    int fd = open(SERVER_PIPE, O_WRONLY);
    if (fd == -1) {
        perror("Error al abrir la pipe del servidor");
        exit(EXIT_FAILURE);
    }
    write(fd, msg, sizeof(Request));
    close(fd);
}

// Función para manejar la señal SIGINT (CTRL+C del cliente)
void handle_sigint(int sig) {
    printf("\nSe recibió la señal SIGINT. Limpiando recursos...\n");
    msg.command_type = 6;
    send_command_to_server(&msg);
    unlink(msg.client_pipe);
    exit(0);
}

// Función para manejar la señal SIGTERM (close, remove y CTRL+C del manager)
void handle_sigterm(int sig) {
    printf("\nSe recibió la señal SIGTERM. Cerrando el cliente...\n");
    unlink(msg.client_pipe);
    exit(0);
}

// Configura manejadores personalizados para las señales SIGINT y SIGTERM
void setup_signal_handlers() {
    struct sigaction sa;

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error al configurar SIGINT");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = handle_sigterm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Error al configurar SIGTERM");
        exit(EXIT_FAILURE);
    }
}

// Función para procesar los comandos del usuario
void handle_user_input() {
    char input[512];
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;

    if (strncmp(input, "subscribe ", 10) == 0) {
        msg.command_type = 1;
        strncpy(msg.topic, input + 10, sizeof(msg.topic));
        send_command_to_server(&msg);

    } else if (strcmp(input, "topics") == 0) {
        msg.command_type = 2;
        msg.topic[0] = '\0';
        send_command_to_server(&msg);

    } else if (strcmp(input, "exit") == 0) {
        msg.command_type = 3;
        printf("Cliente: Saliendo...\n");
        send_command_to_server(&msg);
        exit(0);

    } else if (strncmp(input, "unsubscribe ", 12) == 0) {
        msg.command_type = 4;
        strncpy(msg.topic, input + 12, sizeof(msg.topic));
        send_command_to_server(&msg);

    } else if (strncmp(input, "msg ", 4) == 0) {
        char topic[TOPIC_NAME_LEN];
        int duration = 0;
        char mensaje[TAM_MSG];  // Asegúrate de que TAM_MSG sea al menos 300

        // Leer el tópico y la duración, y luego el mensaje completo
        int args = sscanf(input + 4, "%s %d %[^\n]", topic, &duration, mensaje);

        printf("La longitud del mensaje es: %zu\n", strlen(mensaje));

        if (args < 2 && args == 1) {
            // Si no se pasan ambos parámetros (tópico y duración), el mensaje sigue
            strncpy(mensaje, input + 4 + strlen(topic) + 1, TAM_MSG - 1);  // Limita el mensaje a TAM_MSG - 1 para el '\0'
            mensaje[TAM_MSG - 1] = '\0';  // Asegura el fin de la cadena
        }

        // Verificar que el mensaje no exceda el tamaño máximo (300 caracteres)
        if (strlen(mensaje) > 300) {
            printf("Error: El mensaje excede el límite de 300 caracteres.\n");
            return;
        }

        // Copiar los datos a la estructura msg
        strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
        msg.topic[sizeof(msg.topic) - 1] = '\0';  // Asegura el fin de la cadena
        msg.lifetime = duration;

        strncpy(msg.message, mensaje, TAM_MSG - 1);
        msg.message[TAM_MSG - 1] = '\0';  // Asegura el fin de la cadena

        msg.command_type = 5;

        // Enviar el comando al servidor
        send_command_to_server(&msg);
    } else {
        printf("Comando no reconocido. Intente de nuevo.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <usuario>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Llamada a la función que configura los manejadores de señales
    setup_signal_handlers();

    // Asignamos el nombre de usuario y PID al mensaje
    strncpy(msg.username, argv[1], sizeof(msg.username) - 1);
    msg.pid = getpid();
    msg.username[sizeof(msg.username) - 1] = '\0'; // nos aseguramos que el último índice del array es la finalización

    // Usamos el PID para crear el nombre del pipe
    snprintf(msg.client_pipe, sizeof(msg.client_pipe), "client_pipe_%d", msg.pid);
    mkfifo(msg.client_pipe, 0600);

    // Comando para inicio de sesión (0)
    msg.command_type = 0; 
    send_command_to_server(&msg);

    // Creamos el pipe del cliente
    int client_fd = open(msg.client_pipe, O_RDONLY | O_NONBLOCK);
    if (client_fd == -1) {
        perror("Error al abrir la pipe del cliente");
        unlink(msg.client_pipe);
        return EXIT_FAILURE;
    }

    // Bucle infinito para leer y escribir comandos
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds); // limpia el conjunto de descriptores de archivo
        FD_SET(0, &read_fds); // añade la entrada estándar al conjunto.
        FD_SET(client_fd, &read_fds); // añade el descriptor del pipe del cliente al conjunto.

        // Espera actividad en los descriptores de archivo especificados.
        int activity = select(client_fd + 1, &read_fds, NULL, NULL, NULL);

        // Comprueba si ocurrió un error en select
        if (activity == -1) {
            perror("Error en select");
            break;
        }

        // Si hay actividad en la entrada del usuario, se envia el comando
        if (FD_ISSET(0, &read_fds)) {
            handle_user_input();
        }

        // Si hay actividad en la respuesta del servidor, se imprime la respuesta
        if (FD_ISSET(client_fd, &read_fds)) {
            char response[256];
            ssize_t bytes_read = read(client_fd, response, sizeof(response) - 1);
            if (bytes_read > 0) {
                response[bytes_read] = '\0'; // la cadena debe acabarse con el caracter nulo
                printf("%s\n", response);
            }
        }
    }
    return 0;
}
