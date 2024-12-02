#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define SERVER_PIPE "server_pipe"
#define MAX_TOPICS 20
#define TOPIC_NAME_LEN 20
#define MAX_SUBSCRIBERS 10
#define USERNAME_LEN 256
#define MAX_USERS 10
#define MAX_MESSAGES 100
#define TAM_MSG 300

typedef struct {
    char client_pipe[256]; // Descriptor de archivo del pipe para comunicación con el cliente
    char username[USERNAME_LEN]; // Nombre de usuario del cliente
    pid_t pid; // PID del proceso del cliente
} Client;

// STRUCT DE COMUNICACIÓN CON EL CLIENTE
typedef struct {
    char client_pipe[256]; // Descriptor de archivo del pipe para comunicación con el cliente
    int command_type; // Tipo de comando
    char topic[50];  // Campo para almacenar el nombre del tópico
    char username[50]; // Nombre de usuario del cliente
    pid_t pid; // PID del proceso del cliente
    int lifetime; // Lifetime restante
    char message[256]; // Mensaje que se envía
} Response;

typedef struct {
    char name[TOPIC_NAME_LEN]; // Nombre del tópico
    char subscribers[MAX_SUBSCRIBERS][USERNAME_LEN]; // Matriz para almacenar los nombres de usuarios suscritos a un tópico
    int subscriber_count; // Número de suscriptores al tópico.
    int is_locked; // Indicador de si el tópico está bloqueado.
    int has_active_messages;  // Indicador de si el tópico tiene mensajes activos
} Topic;

typedef struct {
    char topic[TOPIC_NAME_LEN]; // Nombre del tópico al que pertenece el mensaje
    char username[USERNAME_LEN]; // Nombre del usuario que envió el mensaje
    char message[TAM_MSG];  // El contenido del mensaje
    int lifetime; // Lifetime restante
    Response msg;
} StoredMessage;


Topic topics[MAX_TOPICS];
Client clients[MAX_USERS]; // Almacena los usuarios conectados
StoredMessage messages[MAX_MESSAGES]; // Almacena los mensajes de los topicos
int topic_count = 0;
int client_count = 0;
int message_count = 0;
pthread_mutex_t mutex; // Declaración del mutex


// Función para enviar un mensaje a un cliente
void send_response(const char *client_pipe, const char *message) {
    int fd = open(client_pipe, O_WRONLY);
    if (fd != -1) {
        write(fd, message, strlen(message) + 1); // +1 para incluir el carácter nulo
        close(fd);
    } else {
        perror("Error al abrir la pipe del cliente");
    }
}


// Función para cerrar todas las conexiones de clientes
void close_all_connections() {
    for (int i = 0; i < client_count; i++) {
        // Enviar la señal SIGTERM a cada cliente para finalizar su proceso
        if (clients[i].pid > 0) {
            kill(clients[i].pid, SIGTERM);
            printf("Se envió SIGTERM a %s (PID: %d)\n", clients[i].username, clients[i].pid);
        }
    }
}


// Manejar la señal SIGINT (CTRL+C) del programa
void handle_sigint(int sig) {
    printf("\nServidor finalizado. Limpiando recursos...\n");
    close_all_connections(); // Cerrar todas las conexiones de clientes
    unlink(SERVER_PIPE); // Eliminar la pipe del servidor
    exit(0); // Salir del programa
}

// AÑADE UN USUARIO A LA LISTA DE USUARIOS CONECTADOS
void add_client(const char *client_pipe, const char *username, pid_t pid) {
    // Verificar si el cliente ya está conectado
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].username, username) == 0) {
            printf("El cliente %s ya está conectado (PID: %d)\n", username, clients[i].pid);
            return; // No agregar el cliente nuevamente
        }
    }

    // Si no está, añadir el cliente
    if (client_count < MAX_USERS) {
        strncpy(clients[client_count].client_pipe, client_pipe, sizeof(clients[client_count].client_pipe) - 1);
        strncpy(clients[client_count].username, username, USERNAME_LEN);
        clients[client_count].pid = pid;
        client_count++;
        printf("Cliente agregado: %s (PID: %d, Pipe: %s)\n", username, pid, client_pipe); // Agregado Pipe en el log
    } else {
        printf("No se puede agregar el cliente %s. Límite máximo de usuarios alcanzado.\n", username);
    }
}


void subscribe_topic(const char *topic_name, const char *client_pipe, const char *username) {
    if (strlen(topic_name) >= TOPIC_NAME_LEN) {
        send_response(client_pipe, "Error: El nombre del tópico excede el máximo de caracteres.");
        return;
    }

    // Buscar el tópico en la lista de tópicos
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].name, topic_name) == 0) {
            // Verificar si el usuario ya está suscrito
            for (int j = 0; j < topics[i].subscriber_count; j++) {
                if (strcmp(topics[i].subscribers[j], username) == 0) {
                    send_response(client_pipe, "Ya estás suscrito al tópico.");
                    return;
                }
            }

            // Si el usuario no está suscrito, agregarlo
            if (topics[i].subscriber_count < MAX_SUBSCRIBERS) {
                strncpy(topics[i].subscribers[topics[i].subscriber_count], username, USERNAME_LEN);
                topics[i].subscribers[topics[i].subscriber_count][USERNAME_LEN - 1] = '\0';
                topics[i].subscriber_count++;

                // Imprimir mensaje en el servidor
                printf("El usuario '%s' se ha suscrito al tópico '%s'.\n", username, topic_name);

                // Almacenar los mensajes en una lista (buffer)
                char all_messages[1024 * MAX_MESSAGES] = "";  // Suponiendo un límite de mensajes
                for (int j = 0; j < message_count; j++) {
                    if (strcmp(messages[j].topic, topic_name) == 0 && messages[j].lifetime > 0) {
                        // Concatenar el mensaje al buffer
                        char message_to_send[1024];
                        snprintf(message_to_send, sizeof(message_to_send), "%s %s %s\n", messages[j].topic, messages[j].username, messages[j].message);
                        strncat(all_messages, message_to_send, sizeof(all_messages) - strlen(all_messages) - 1);
                    }
                }

                // Enviar todos los mensajes de una vez
                if (strlen(all_messages) > 0) {
                    send_response(client_pipe, all_messages);
                }

                // Informar a los suscriptores actuales del tópico
                printf("Usuarios suscritos al tópico '%s':\n", topic_name);
                for (int j = 0; j < topics[i].subscriber_count; j++) {
                    printf(" - %s\n", topics[i].subscribers[j]);
                }

                send_response(client_pipe, "Te has suscrito al tópico.");
            } else {
                send_response(client_pipe, "Error: máximo de suscriptores alcanzado.");
            }
            return;
        }
    }

    // Si el tópico no existe, crear uno nuevo
    if (topic_count < MAX_TOPICS) {
        // Crear un nuevo tópico y agregar al primer suscriptor
        strncpy(topics[topic_count].name, topic_name, TOPIC_NAME_LEN);
        topics[topic_count].name[TOPIC_NAME_LEN - 1] = '\0';
        topics[topic_count].subscriber_count = 0;

        // Agregar el primer suscriptor (el usuario que se suscribe)
        strncpy(topics[topic_count].subscribers[0], username, USERNAME_LEN);
        topics[topic_count].subscriber_count++;

        // Imprimir mensaje en el servidor
        printf("El usuario '%s' ha creado y se ha suscrito al tópico '%s'.\n", username, topic_name);

        topic_count++;

        // Enviar respuesta al cliente
        send_response(client_pipe, "Tópico creado y suscrito.");

        // Almacenar los mensajes en una lista (buffer)
        char all_messages[1024 * MAX_MESSAGES] = "";  // Suponiendo un límite de mensajes
        for (int i = 0; i < message_count; i++) {
            if (strcmp(messages[i].topic, topic_name) == 0 && messages[i].lifetime > 0) {
                // Concatenar el mensaje al buffer
                char message_to_send[1024];
                snprintf(message_to_send, sizeof(message_to_send), "%s %s %s\n", messages[i].topic ,messages[i].username, messages[i].message);
                strncat(all_messages, message_to_send, sizeof(all_messages) - strlen(all_messages) - 1);
            }
        }

        // Enviar todos los mensajes de una vez
        if (strlen(all_messages) > 0) {
            send_response(client_pipe, all_messages);
        }
    } else {
        send_response(client_pipe, "Error: máximo de tópicos alcanzado.");
    }
}


void unsubscribe_topic(const char *topic_name, const char *client_pipe, const char *username) {
    // 1. Recorre todos los tópicos para encontrar el tópico al que el usuario desea desuscribirse
    for (int i = 0; i < topic_count; i++) {
        // 2. Verifica si el nombre del tópico coincide con el tópico que el usuario quiere desuscribirse
        if (strcmp(topics[i].name, topic_name) == 0) {
            
            // 3. Recorre los suscriptores del tópico
            for (int j = 0; j < topics[i].subscriber_count; j++) {
                
                // 4. Verifica si el usuario está suscrito a este tópico
                if (strcmp(topics[i].subscribers[j], username) == 0) {
                    
                    // 5. Si el usuario está suscrito, lo elimina de la lista de suscriptores del tópico
                    // Empezamos desde el índice del suscriptor y recorre los suscriptores detrás de él
                    for (int k = j; k < topics[i].subscriber_count - 1; k++) {
                        // Desplaza los suscriptores restantes una posición hacia atrás para sobrescribir al usuario eliminado
                        // Se copia el suscriptor siguiente en la posición donde estaba el suscriptor eliminado y hace lo mismo con los demás
                        strncpy(topics[i].subscribers[k], topics[i].subscribers[k + 1], USERNAME_LEN);
                    }
                    
                    // 6. Disminuye el contador de suscriptores para reflejar la eliminación
                    topics[i].subscriber_count--;
                    
                    // 7. Envia una respuesta al cliente confirmando que se desuscribió correctamente
                    send_response(client_pipe, "Te has desuscrito del tópico.");
                    return;
                }
            }

            // Si el usuario no estaba suscrito al tópico, envía una respuesta al cliente
            send_response(client_pipe, "No estás suscrito al tópico.");
            return;
        }
    }

    // Si no se encuentra el tópico en la lista, se envía una respuesta indicando que el tópico no existe
    send_response(client_pipe, "El tópico no existe.");
}



// Función que lista los topicos
void list_topics(const char *client_pipe) {
    char response[1024] = "Tópicos:\n";
    for (int i = 0; i < topic_count; i++) {
        char topic_info[100];
        snprintf(topic_info, sizeof(topic_info), "- %s (Suscriptores: %d)\n", topics[i].name, topics[i].subscriber_count);
        strcat(response, topic_info);
    }
    printf("Se listaron %d tópicos.\n", topic_count);
    send_response(client_pipe, response);
}


// Función para verificar si un tópico existe
int topic_exists(const char *topic_name) {  
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].name, topic_name) == 0) {
            return 1; // El tópico existe
        }
    }
    return 0; // El tópico no existe
}

// Lista los usuarios conectados
void list_connected_users() {
    if (client_count == 0) {
        printf("No hay usuarios conectados.\n");
        return; // Salir de la función si no hay usuarios conectados
    }

    for (int i = 0; i < client_count; i++) {
        printf("- %s (Pipe: %s)\n", clients[i].username, clients[i].client_pipe);
    }
}


void send_message(Response* request) {
    // Verificar si el tópico existe
    int topic_index = -1;
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].name, request->topic) == 0) {
            topic_index = i;
            break;
        }
    }

    // Si el tópico no existe, crearlo
    if (topic_index == -1) {
        if (topic_count < MAX_TOPICS) {
            // Inicializar el nuevo tópico
            strncpy(topics[topic_count].name, request->topic, TOPIC_NAME_LEN);
            topics[topic_count].name[TOPIC_NAME_LEN - 1] = '\0';
            topics[topic_count].subscriber_count = 0; // Sin suscriptores iniciales
            topics[topic_count].is_locked = 0;       // No bloqueado por defecto
            topics[topic_count].has_active_messages = 0; // Sin mensajes activos inicialmente

            topic_index = topic_count;
            topic_count++;

            printf("Tópico '%s' creado automáticamente.\n", request->topic);
        } else {
            send_response(request->client_pipe, "Error: No se pueden crear más tópicos, límite alcanzado.");
            return;
        }
    }

    // Verificar si el tópico está bloqueado
    if (topics[topic_index].is_locked) {
        send_response(request->client_pipe, "El tópico está bloqueado. No se puede enviar el mensaje.");
        return;
    }

    // Verificar si el cuerpo del mensaje no excede los 300 caracteres
    if (strlen(request->message) > TAM_MSG) {
        send_response(request->client_pipe, "Error: El mensaje excede el límite de 300 caracteres.");
        return;
    }

    // Si el mensaje es persistente, verificar el número de mensajes persistentes en el tópico
    if (request->lifetime > 0) {
        int persistent_message_count = 0;

        // Contar los mensajes persistentes en el tópico
        for (int i = 0; i < message_count; i++) {
            if (strcmp(messages[i].topic, request->topic) == 0 && messages[i].lifetime > 0) {
                persistent_message_count++;
            }
        }

        // Verificar si se ha alcanzado el límite de 5 mensajes persistentes
        if (persistent_message_count >= 5) {
            send_response(request->client_pipe, "Error: Se ha alcanzado el límite de 5 mensajes persistentes en este tópico.");
            return;
        }
    }

    // Almacenar el mensaje
    if (message_count < MAX_MESSAGES) {
        // Guardar el mensaje en la estructura de mensajes
        strncpy(messages[message_count].topic, request->topic, sizeof(messages[message_count].topic) - 1);
        strncpy(messages[message_count].username, request->username, sizeof(messages[message_count].username) - 1);
        strncpy(messages[message_count].message, request->message, sizeof(messages[message_count].message) - 1);
        messages[message_count].lifetime = request->lifetime; // Lifetime restante
        message_count++;

        // Marcar que el tópico ahora tiene mensajes activos
        topics[topic_index].has_active_messages = 1;
        // Enviar el mensaje a los suscriptores excepto al remitente
        char formatted_message[1028]; // Espacio para el formato
        snprintf(formatted_message, sizeof(formatted_message), "%s %s %d %s",
         request->topic, request->username, request->lifetime, request->message);

        // Enviar el mensaje a los suscriptores excepto al remitente
        for (int i = 0; i < topics[topic_index].subscriber_count; i++) {
            const char *subscriber_username = topics[topic_index].subscribers[i];
            if (strcmp(subscriber_username, request->username) != 0) { // Evitar al remitente
                for (int j = 0; j < client_count; j++) {
                    if (strcmp(clients[j].username, subscriber_username) == 0) {
                        send_response(clients[j].client_pipe, formatted_message);
                    }
                }
            }
        }

        // Guardar el mensaje en el archivo si es persistente
        if (request->lifetime > 0) {
            const char* msg_file = getenv("MSG_FICH");
            if (msg_file) {
                FILE* file = fopen(msg_file, "a");
                if (file) {
                    fprintf(file, "%s %s %d %s\n",
                            request->topic, request->username, request->lifetime, request->message);
                    fclose(file);
                } else {
                    perror("Error al abrir el archivo de mensajes");
                }
            } else {
                perror("Variable de entorno MSG_FICH no configurada");
            }
        }

        // Imprimir el mensaje en la consola
        printf("Mensaje de %s enviado al tópico %s\n", request->username, request->topic);

        // Enviar una respuesta al cliente que envió el mensaje
        send_response(request->client_pipe, "Mensaje enviado con éxito.");
    } else {
        send_response(request->client_pipe, "Error: máximo de mensajes alcanzado.");
    }
}



// Función para cargar los mensajes cuyo lifetime sea mayor a 0 desde el archivo
int load_messages() {
    const char* msg_file = getenv("MSG_FICH"); // Obtener el archivo desde la variable de entorno
    if (!msg_file) {
        perror("Variable de entorno MSG_FICH no configurada");
        return 0; // Si no se puede obtener la variable de entorno, devolver 0
    }

    FILE* file = fopen(msg_file, "r"); // Abrir el archivo para lectura
    if (!file) {
        perror("Error al abrir el archivo de mensajes para lectura");
        return 0; // Si no se puede abrir el archivo, devolver 0
    }

    int loaded_count = 0;
    while (fscanf(file, "%s %s %d %[^\n]", 
                   messages[loaded_count].topic, 
                   messages[loaded_count].username, 
                   &messages[loaded_count].lifetime, 
                   messages[loaded_count].message) == 4) {
        // Solo cargar los mensajes cuyo lifetime sea mayor a 0
        if (messages[loaded_count].lifetime > 0) {
            // Verificar si el tópico ya existe
            int topic_exists = 0;
            for (int i = 0; i < topic_count; i++) {
                if (strcmp(topics[i].name, messages[loaded_count].topic) == 0) {
                    topic_exists = 1;
                    topics[i].has_active_messages = 1; // Marcamos que el tópico tiene mensajes activos
                    break;
                }
            }

            // Si el tópico no existe, agregarlo
            if (!topic_exists) {
                // Añadir un nuevo tópico al arreglo
                strcpy(topics[topic_count].name, messages[loaded_count].topic);
                topics[topic_count].has_active_messages = 1; // Tópico con mensaje activo
                topic_count++;
            }

            loaded_count++; // Incrementar el contador si el mensaje es válido
        }
    }

    fclose(file); // Cerrar el archivo después de leer
    return loaded_count; // Retornar el número de mensajes cargados
}


void* manage_lifetime(void* arg) {
    // Cargar los mensajes cuyo lifetime sea mayor a 0
    message_count = load_messages();  // Cargar mensajes desde el archivo

    // Ejecutar el bucle para disminuir el lifetime de los mensajes
    while (1) {
        sleep(1);  // Esperar 1 segundo para actualizar el archivo

        // Decrementar el lifetime de los mensajes en memoria
        for (int i = 0; i < message_count; i++) {
            if (messages[i].lifetime > 0) {
                messages[i].lifetime--;  // Decrementar el lifetime
            }
        }

        // Eliminar los mensajes con lifetime == 0
        int new_message_count = 0;
        for (int i = 0; i < message_count; i++) {
            if (messages[i].lifetime > 0) {
                // Si el mensaje tiene lifetime > 0, mantenerlo
                messages[new_message_count] = messages[i];
                new_message_count++;
            }
        }
        message_count = new_message_count;  // Actualizar el contador de mensajes

        // Verificar si algún tópico tiene mensajes activos
        for (int i = 0; i < topic_count; i++) {
            int topic_has_active_messages = 0;

            // Comprobar si el tópico tiene mensajes con lifetime > 0
            for (int j = 0; j < message_count; j++) {
                if (strcmp(topics[i].name, messages[j].topic) == 0 && messages[j].lifetime > 0) {
                    topic_has_active_messages = 1;
                    break; // Si encontramos un mensaje activo, no eliminar el tópico
                }
            }

            // Si el tópico no tiene mensajes activos, marcarlo como no activo
            topics[i].has_active_messages = topic_has_active_messages;
        }

        // Eliminar tópicos sin mensajes activos y sin suscriptores
        for (int i = 0; i < topic_count; i++) {
            if (!topics[i].has_active_messages && topics[i].subscriber_count == 0) {
                // Si el tópico no tiene mensajes activos y no tiene suscriptores, eliminarlo
                for (int j = i; j < topic_count - 1; j++) {
                    topics[j] = topics[j + 1];  // Desplazar todos los tópicos después de la posición i
                }
                topic_count--;  // Reducir el contador de tópicos
                i--;  // Ajustar el índice para revisar el nuevo tópico en la posición i
            }
        }

        // Obtener la ruta del archivo desde la variable de entorno
        const char* msg_file = getenv("MSG_FICH");
        if (!msg_file) {
            perror("Variable de entorno MSG_FICH no configurada");
            return NULL;
        }

        // Reescribir el archivo solo con los mensajes que aún tienen lifetime > 0 (se borran los de lifetime = 0)
        FILE* file = fopen(msg_file, "w"); // Abrir el archivo para escritura
        if (file) {
            for (int i = 0; i < message_count; i++) {
                if (messages[i].lifetime > 0) { // elige solo los mensajes con lifetime mayor a 0
                    fprintf(file, "%s %s %d %s\n",
                            messages[i].topic,
                            messages[i].username,
                            messages[i].lifetime,
                            messages[i].message);
                }
            }
            fclose(file);  // Cerrar el archivo después de actualizar
        } else {
            perror("Error al abrir el archivo de mensajes para reescritura");
        }
    }
    pthread_exit(NULL);
}



void remove_client(const char *username) {
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].username, username) == 0) {
            // Enviar la señal SIGTERM al proceso del cliente para finalizar su proceso
            if (clients[i].pid > 0) {
                kill(clients[i].pid, SIGTERM);
                printf("Se envió SIGTERM a %s (PID: %d)\n", username, clients[i].pid);
            }
            // Desplazar elementos hacia atrás para eliminar al cliente
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--; // Reducir el contador de clientes
            printf("Cliente '%s' ha sido eliminado de la lista de conectados.\n", username);
            char formatted_message[100];
            snprintf(formatted_message, sizeof(formatted_message), "El cliente '%s' ha sido eliminado de la lista de conectados.\n", username);  
            //notificarr a loss users conecctado
            for (int i = 0; i < client_count; i++) {
                send_response(clients[i].client_pipe, formatted_message);
            }
            return;
        }
    }
    printf("Cliente '%s' no encontrado.\n", username); // Mensaje si el usuario no está conectado
}


void show_messages(const char *topic_name) {
    // Comprobar si el tópico existe
    if (!topic_exists(topic_name)) {
        printf("El tópico '%s' no existe.\n", topic_name);
        return; // Salir de la función si el tópico no existe
    }

    int found_messages = 0; // Contador para verificar si hay mensajes
    const char *msg_file = getenv("MSG_FICH");  // Obtener el nombre del archivo desde la variable de entorno

    if (msg_file == NULL) {
        printf("La variable de entorno MSG_FICH no está configurada.\n");
        return;
    }

    FILE *file = fopen(msg_file, "r");  // Abrir el archivo en modo lectura
    if (file == NULL) {
        perror("Error al abrir el archivo de mensajes");
        return;
    }

    char line[512];  // Buffer para leer cada línea del archivo
    while (fgets(line, sizeof(line), file)) {
        StoredMessage msg;
        // Leer los datos de la línea (suponiendo el formato "topic username lifetime message")
        int n = sscanf(line, "%255s %255s %d %300[^\n]", msg.topic, msg.username, &msg.lifetime, msg.message);
        if (n != 4) {
            continue;  // Si la línea no tiene el formato correcto, pasar a la siguiente
        }

        // Comprobar si el mensaje pertenece al tópico dado
        if (strcmp(msg.topic, topic_name) == 0) {
            found_messages = 1; // Se encontraron mensajes
            printf("Usuario: %s, Mensaje: %s\n", msg.username, msg.message);  // Imprimir información del mensaje
        }
    }

    fclose(file);  // Cerrar el archivo

    if (!found_messages) {
        printf("No hay mensajes en el tópico '%s'.\n", topic_name); // Imprimir si no hay mensajes
    }
}



void handle_ctrlc(const char *username) {
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].username, username) == 0) {
            // Enviar la señal SIGTERM al proceso del cliente para finalizar su proceso
            if (clients[i].pid > 0) {
                kill(clients[i].pid, SIGINT);
                printf("Se envió SIGINT a %s (PID: %d)\n", username, clients[i].pid);
            }
            // Desplazar elementos hacia atrás para eliminar al cliente
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--; // Reducir el contador de clientes
            printf("Cliente '%s' ha sido eliminado de la lista de conectados.\n", username);
            return;
        }
    }
    printf("Cliente '%s' no encontrado.\n", username); // Mensaje si el usuario no está conectado
}

void lock_topic(const char *topic_name) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].name, topic_name) == 0) {
            if (!topics[i].is_locked) {
                topics[i].is_locked = 1; // Bloquear el tópico
                printf("Tópico '%s' bloqueado.\n", topic_name);

                // Notificar a los suscriptores del bloqueo
                char notification[256];
                snprintf(notification, sizeof(notification), "El tópico '%s' ha sido bloqueado. No se pueden enviar mensajes temporalmente.", topic_name);

                for (int j = 0; j < topics[i].subscriber_count; j++) {
                    // Buscar al cliente correspondiente al suscriptor
                    for (int k = 0; k < client_count; k++) {
                        if (strcmp(clients[k].username, topics[i].subscribers[j]) == 0) {
                            send_response(clients[k].client_pipe, notification);
                            break;
                        }
                    }
                }
            } else {
                printf("El tópico '%s' ya está bloqueado.\n", topic_name);
            }
            return;
        }
    }
    printf("No se encontró el tópico '%s' para bloquear.\n", topic_name);
}


void unlock_topic(const char* topic_name) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].name, topic_name) == 0) {
            if (topics[i].is_locked) { // Verificar si el tópico está bloqueado
                topics[i].is_locked = 0;  // Desbloquear el tópico
                printf("El tópico '%s' ha sido desbloqueado para el envío de mensajes.\n", topic_name);

                // Notificar a los suscriptores del desbloqueo
                char notification[256];
                snprintf(notification, sizeof(notification), "El tópico '%s' ha sido desbloqueado. Ya puedes enviar mensajes.", topic_name);

                for (int j = 0; j < topics[i].subscriber_count; j++) {
                    // Buscar al cliente correspondiente al suscriptor
                    for (int k = 0; k < client_count; k++) {
                        if (strcmp(clients[k].username, topics[i].subscribers[j]) == 0) {
                            send_response(clients[k].client_pipe, notification);
                            break;
                        }
                    }
                }
            } else {
                printf("El tópico '%s' ya está desbloqueado.\n", topic_name);
            }
            return;
        }
    }
    printf("Error: Tópico '%s' no encontrado.\n", topic_name);
}


void *command_sender(void *arg) {
    char input[256];
    while (1) {
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0; // Eliminar el salto de línea al final

        if (strncmp(input, "remove ", 7) == 0) {
            // Extraer el nombre del usuario
            char username[USERNAME_LEN];
            sscanf(input + 7, "%s", username); // Obtener el username a partir del input
            
            pthread_mutex_lock(&mutex); // Bloquear el mutex al modificar los usuarios
            remove_client(username); // Llamar a remove_client
            pthread_mutex_unlock(&mutex);
        } 
        else if (strcmp(input, "close") == 0) {
            close_all_connections(); // Cerrar todas las conexiones
            unlink(SERVER_PIPE); // Limpiar la pipe del servidor
            exit(0); // Terminar el servidor
        }
        else if (strcmp(input, "users") == 0) {
            pthread_mutex_lock(&mutex); // Bloquear el mutex al acceder a los usuarios
            printf("Lista de usuarios conectados:\n");
            list_connected_users(); // Listar usuarios conectados
            pthread_mutex_unlock(&mutex);
        }
        else if (strcmp(input, "topics") == 0) {
            pthread_mutex_lock(&mutex); // Bloquear el mutex al acceder a los tópicos
            printf("Tópicos:\n");
            for (int i = 0; i < topic_count; i++) {
                printf(" - %s (Suscriptores: %d)\n", topics[i].name, topics[i].subscriber_count);
            }
            pthread_mutex_unlock(&mutex);
        } 
        else if (strncmp(input, "lock ", 5) == 0) {
            char topico[TOPIC_NAME_LEN];
            sscanf(input + 5, "%s", topico); // Capturar el nombre del tópico
            pthread_mutex_lock(&mutex); // Bloquear el mutex 
            lock_topic(topico);
            pthread_mutex_unlock(&mutex);
        }
        else if (strncmp(input, "unlock ", 7) == 0) {
            char topico[TOPIC_NAME_LEN];
            sscanf(input + 7, "%s", topico); // Capturar el nombre del tópico
            pthread_mutex_lock(&mutex); // Bloquear el mutex 
            unlock_topic(topico);
            pthread_mutex_unlock(&mutex);
        }
        else if (strncmp(input, "show", 4) == 0){
            printf("Mensajes del tópico:\n");
            char topico[TOPIC_NAME_LEN];
            sscanf(input + 4, "%s", topico); // Capturar el nombre del tópico
            pthread_mutex_lock(&mutex); // Bloquear el mutex 
            show_messages(topico);
            pthread_mutex_unlock(&mutex);
        }
        else {
            printf("No existe este comando: %s\n", input);
            continue;
        }
    }
}



int main() {
    Response msg;
    load_messages();

    // Configurar el manejador de señal para SIGINT
    signal(SIGINT, handle_sigint);

    if (access(SERVER_PIPE, F_OK) == 0){
        printf("YA HAY UN SERVIDOR EN EJECUCIÓN\n");
        exit(1);
    }

    // Crear la pipe del servidor
    mkfifo(SERVER_PIPE, 0600);

    pthread_t lifetime_thread;
    // Iniciar el hilo para gestionar el lifetime de los mensajes
    if (pthread_create(&lifetime_thread, NULL, manage_lifetime, NULL) != 0) {
        perror("Error al crear el hilo de gestión de lifetime");
        return 1;
    }

    // Crea un hilo para ejecutar los comandos ya que el hilo principal escucha los comandos del cliente
    pthread_mutex_init(&mutex, NULL); // Inicializar el mutex
    pthread_t command_thread;
    pthread_create(&command_thread, NULL, command_sender, NULL); // Crear hilo para comandos

    printf("Esperando conexiones...\n");

    while (1) {
            // Esperar por un mensaje del cliente
            int fd = open(SERVER_PIPE, O_RDONLY);
            if (fd == -1) {
                perror("Error al abrir la pipe del servidor");
                continue; // Volver a intentar en el siguiente ciclo
            }

            ssize_t bytesRead = read(fd, &msg, sizeof(Response));
            if (bytesRead < 0) {
                perror("Error al leer el mensaje del cliente");
                close(fd);
                continue; // Volver a intentar en el siguiente ciclo
            } else if (bytesRead == 0) {
                close(fd); // USO ESTO PARA QUE NO SALGA DUPLICADO EL MENSAJE DE CONEXION DEL USUARIO
                continue; // Volver a intentar en el siguiente ciclo
            }

            // Agregar lógica para manejar diferentes tipos de comandos
            pthread_mutex_lock(&mutex);

            switch (msg.command_type) {
                case 0: // Mensaje de conexión
                    char res[512];
                    if (client_count < MAX_USERS) {
                        int duplicate_found = 0; 
                        // Verificar si el nombre de usuario ya está en uso
                        for (int i = 0; i < client_count; i++) {
                            if (strcmp(clients[i].username, msg.username) == 0) {
                                printf("ERR: Username '%s' is already in use.\n", msg.username);
                                duplicate_found = 1;
                                sprintf(res, "ERR: Username '%s' is already in use.\n", msg.username);
                                send_response(msg.client_pipe, res);
                                sleep(1);
                                kill(msg.pid, SIGTERM); //cierra feed
                            }
                        }

                        // Si no se encuentra un duplicado, agregar al nuevo cliente
                        if (duplicate_found == 0) {
                            if (msg.username[0] != '\0') { // Verificar que el nombre no esté vacío
                                sprintf(res, "Bienvenido, %s", msg.username);
                                send_response(msg.client_pipe, res);
                                add_client(msg.client_pipe, msg.username, msg.pid); // Asegúrate de pasar correctamente los datos
                            } else {
                                printf("ERR: Invalid username.\n");
                                send_response(msg.client_pipe, "ERR: Invalid username.\n");
                                sleep(1);
                                kill(msg.pid, SIGTERM); 
                            }
                        }
                    } else {            
                        printf("ERR: Max number of users reached (%d).\n", MAX_USERS);
                        sprintf(res, "ERR: Max number of users reached (%d).\n", MAX_USERS);
                        send_response(msg.client_pipe, res);
                        sleep(1);
                        kill(msg.pid, SIGTERM);
                    }
                break;

                case 1: // Crear tópico
                    subscribe_topic(msg.topic, msg.client_pipe, msg.username);
                    break;
                
                case 2: // Listar tópicos
                    printf("Listar tópicos para el usuario '%s'.\n", msg.username);
                    list_topics(msg.client_pipe);
                    break;
                
                case 3: // Salir
                    printf("Cliente '%s' ha salido.\n", msg.username);
                    remove_client(msg.username);
                    break;
                
                case 4: // Desuscribirse del tópico
                    printf("Desuscribirse del tópico '%s' para el usuario '%s'.\n", msg.topic, msg.username);
                    unsubscribe_topic(msg.topic, msg.client_pipe, msg.username);
                    break;

                case 5: // Enviar mensaje y guardarlo en un archivo de texto si es persistente
                    send_message(&msg);
                    break;

                case 6:
                    lock_topic(msg.topic);
                    break;

                case 7:
                    unlock_topic(msg.topic);
                    break;

                case 9: // CTRL+C del cliente
                    handle_ctrlc(msg.username);
                    break;
                
                default:
                    // Enviar respuesta de comando no reconocido
                    int client_fd = open(msg.client_pipe, O_WRONLY);
                    write(client_fd, "Comando no reconocido.", 22);
                    close(client_fd);
                    printf("Comando no reconocido: tipo %d\n", msg.command_type);
                    break;
            }

            pthread_mutex_unlock(&mutex); // Desbloquear el mutex después de acceder a recursos compartidos
        }
        // Espera a que el hilo actual termine
        pthread_join(lifetime_thread, NULL);
        return 0;
}
