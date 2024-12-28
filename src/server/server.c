#include <arpa/inet.h>
#include <json-c/json.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../utils/config.h"
#include "../utils/helper.h"
#include "../utils/structs.h"
#include "./handler/file_handler.h"
#include "./handler/folder_handler.h"
#include "./handler/user_handler.h"
#include "db/db_access.h"

// Function prototypes
void *handle_client(void *arg);

int main()
{
    Config *config = get_config();

    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t tid[MAX_CLIENTS];
    int client_count = 0;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(config->server_port);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", config->server_port);

    // Accept and handle client connections
    while (1)
    {
        client_t *client = malloc(sizeof(client_t));
        client->is_logged_in = 0;

        if ((client->socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept failed");
            free(client);
            continue;
        }

        // Create new thread for client
        if (pthread_create(&tid[client_count++], NULL, handle_client, (void *)client) != 0)
        {
            perror("Failed to create thread");
            close(client->socket);
            free(client);
            continue;
        }

        // Limit check
        if (client_count >= MAX_CLIENTS)
        {
            printf("Max clients reached. Waiting for some to disconnect...\n");
            pthread_join(tid[0], NULL);
            client_count--;
        }
    }

    return 0;
}

void *handle_client(void *arg)
{
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(client->socket, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[read_size] = '\0';
        struct json_object *parsed_json = json_tokener_parse(buffer);
        struct json_object *message_type;

        json_object_object_get_ex(parsed_json, "messageType", &message_type);
        const char *type = json_object_get_string(message_type);

        printf("Request from client: %s\n", strdup(buffer));

        if (!client->is_logged_in)
        {
            if (strcmp(type, "LOGIN") == 0)
            {
                handle_login(client, buffer);
            }
            else if (strcmp(type, "REGISTER") == 0)
            {
                handle_register(client, buffer);
            }
            else
                send_response(client->socket, 401, "Unauthorized");
        }
        else
        {
            // Folder operations
            // TODO: Add FOLDER_CONTENT handler
            if (strcmp(type, "FOLDER_CREATE") == 0)
            {
                handle_folder_create(client, buffer);
            }
            else if (strcmp(type, "FOLDER_RENAME") == 0)
            {
                handle_folder_rename(client, buffer);
            }
            else if (strcmp(type, "FOLDER_COPY") == 0)
            {
                handle_folder_copy(client, buffer);
            }
            else if (strcmp(type, "FOLDER_MOVE") == 0)
            {
                handle_folder_move(client, buffer);
            }
            else if (strcmp(type, "FOLDER_DELETE") == 0)
            {
                handle_folder_delete(client, buffer);
            }
            else if (strcmp(type, "SET_FOLDER_ACCESS") == 0)
            {
                handle_folder_set_access(client, buffer);
            }
            else if (strcmp(type, "SET_FILE_ACCESS") == 0)
            {
                handle_file_set_access(client, buffer);
            }
            else if (strcmp(type, "FOLDER_SEARCH") == 0)
            {
                handle_folder_search(client, buffer);
            }
            else if (strcmp(type, "FOLDER_DOWNLOAD") == 0)
            {
                handle_folder_download(client, buffer);
            }
            else if (strcmp(type, "FOLDER_UPLOAD") == 0)
            {
                handle_folder_upload(client, buffer);
            }
            // File operations
            else if (strcmp(type, "FILE_UPLOAD") == 0)
            {
                handle_file_create(client, buffer);
            }
            else if (strcmp(type, "FILE_DOWNLOAD") == 0)
            {
                handle_file_download(client, buffer);
            }
            else if (strcmp(type, "FILE_RENAME") == 0)
            {
                handle_file_rename(client, buffer);
            }
            else if (strcmp(type, "FILE_COPY") == 0)
            {
                handle_file_copy(client, buffer);
            }
            else if (strcmp(type, "FILE_MOVE") == 0)
            {
                handle_file_move(client, buffer);
            }
            else if (strcmp(type, "FILE_DELETE") == 0)
            {
                handle_file_delete(client, buffer);
            }
            else if (strcmp(type, "FILE_SEARCH") == 0)
            {
                handle_file_search(client, buffer);
            }
            else if (strcmp(type, "LOGOUT") == 0)
            {
                client->is_logged_in = 0;
                client->username[0] = '\0';
                send_response(client->socket, 200, "Logged out");
            }
            else
            {
                send_response(client->socket, 404, "Invalid request");
            }
        }

        json_object_put(parsed_json);
    }

    close(client->socket);
    free(client);
    return NULL;
}