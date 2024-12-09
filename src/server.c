#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <json-c/json.h>

#define PORT 5555
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 10
#define MAX_USERNAME 32
#define MAX_PASSWORD 32

typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} user_t;

typedef struct {
    int socket;
    char username[MAX_USERNAME];
    int is_logged_in;
} client_t;

// Simple user database (in memory)
user_t users[100];
int user_count = 0;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void *handle_client(void *arg);
void handle_login(client_t *client, const char *buffer);
void handle_register(client_t *client, const char *buffer);
void send_response(int socket, int code, const char *message);

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t tid[MAX_CLIENTS];
    int client_count = 0;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept and handle client connections
    while (1) {
        client_t *client = malloc(sizeof(client_t));
        client->is_logged_in = 0;
        
        if ((client->socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            free(client);
            continue;
        }

        // Send connection success response
        send_response(client->socket, 200, "Connected to server");

        // Create new thread for client
        if (pthread_create(&tid[client_count++], NULL, handle_client, (void*)client) != 0) {
            perror("Failed to create thread");
            close(client->socket);
            free(client);
            continue;
        }

        // Limit check
        if (client_count >= MAX_CLIENTS) {
            printf("Max clients reached. Waiting for some to disconnect...\n");
            pthread_join(tid[0], NULL);
            client_count--;
        }
    }

    return 0;
}

void *handle_client(void *arg) {
    client_t *client = (client_t*)arg;
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(client->socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';

        // Parse JSON message
        struct json_object *parsed_json = json_tokener_parse(buffer);
        struct json_object *message_type;

        json_object_object_get_ex(parsed_json, "messageType", &message_type);
        const char *type = json_object_get_string(message_type);

        if (strcmp(type, "LOGIN") == 0) {
            handle_login(client, buffer);
        }
        else if (strcmp(type, "REGISTER") == 0) {
            handle_register(client, buffer);
        }
        else if (strcmp(type, "EXIT") == 0) {
            break;
        }

        json_object_put(parsed_json);
    }

    close(client->socket);
    free(client);
    return NULL;
}

void handle_login(client_t *client, const char *buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *username_obj, *password_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "userName", &username_obj);
    json_object_object_get_ex(payload, "password", &password_obj);
    
    const char *username = json_object_get_string(username_obj);
    const char *password = json_object_get_string(password_obj);

    pthread_mutex_lock(&users_mutex);
    int found = 0;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && 
            strcmp(users[i].password, password) == 0) {
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);

    if (found) {
        strncpy(client->username, username, MAX_USERNAME - 1);
        client->is_logged_in = 1;
        send_response(client->socket, 200, "Login successful");
    } else {
        send_response(client->socket, 401, "Invalid credentials");
    }

    json_object_put(parsed_json);
}

void handle_register(client_t *client, const char *buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *username_obj, *password_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "userName", &username_obj);
    json_object_object_get_ex(payload, "password", &password_obj);
    
    const char *username = json_object_get_string(username_obj);
    const char *password = json_object_get_string(password_obj);

    pthread_mutex_lock(&users_mutex);
    int exists = 0;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            exists = 1;
            break;
        }
    }

    if (!exists && user_count < 100) {
        strncpy(users[user_count].username, username, MAX_USERNAME - 1);
        strncpy(users[user_count].password, password, MAX_PASSWORD - 1);
        user_count++;
        
        strncpy(client->username, username, MAX_USERNAME - 1);
        client->is_logged_in = 1;
        send_response(client->socket, 201, "Registration successful");
    } else {
        send_response(client->socket, 409, "User already exists");
    }
    pthread_mutex_unlock(&users_mutex);

    json_object_put(parsed_json);
}

void send_response(int socket, int code, const char *message) {
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "{\"responseCode\": %d, \"message\": \"%s\"}", code, message);
    send(socket, response, strlen(response), 0);
} 