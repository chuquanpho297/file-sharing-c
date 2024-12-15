#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <json-c/json.h>
#include <mysql/mysql.h>
#include "../utils/config.h"
#include "../utils/structs.h"
#include "db/db_access.h"
#include "file_handler.h"
#include "group_handler.h"

// Database credentials
const char *host = DB_HOST;
const char *user = DB_USER;
const char *password = DB_PASS;
const char *db_name = DB_NAME;
const unsigned int port = DB_PORT;

typedef struct
{
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} user_t;

pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void *handle_client(void *arg);
void handle_login(client_t *client, const char *buffer);
void handle_register(client_t *client, const char *buffer);
void send_response(int socket, int code, const char *message);
void handle_create_group(client_t* client, const char* buffer);
void handle_list_all_groups(client_t* client, const char* buffer);
void handle_join_group(client_t* client, const char* buffer);
void handle_invite_to_group(client_t* client, const char* buffer);
void handle_leave_group(client_t* client, const char* buffer);
void handle_list_group_members(client_t* client, const char* buffer);
void handle_list_invitations(client_t* client, const char* buffer);
void handle_remove_member(client_t* client, const char* buffer);
void handle_approval(client_t* client, const char* buffer);
void handle_join_request_status(client_t* client, const char* buffer);
void handle_join_request_list(client_t* client, const char* buffer);
void handle_folder_content(client_t* client, const char* buffer);
void handle_create_folder(client_t* client, const char* buffer);
void handle_folder_rename(client_t* client, const char* buffer);
void handle_folder_copy(client_t* client, const char* buffer);
void handle_folder_move(client_t* client, const char* buffer);
void handle_folder_delete(client_t* client, const char* buffer);

int main()
{

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
    address.sin_port = htons(PORT);

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

    printf("Server listening on port %d...\n", PORT);

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

        // Send connection success response
        send_response(client->socket, 200, "Connected to server");

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

    // Send initial connection response
    // send_response(client->socket, 200, "Connected to server");

    while ((read_size = recv(client->socket, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[read_size] = '\0';

        struct json_object *parsed_json = json_tokener_parse(buffer);
        struct json_object *message_type;

        json_object_object_get_ex(parsed_json, "messageType", &message_type);
        const char *type = json_object_get_string(message_type);

        printf("Request from client: %s\n", buffer);

        if (strcmp(type, "EXIT") == 0)
        {
            printf("Client disconnected\n");
            break;
        }

        // Group operations
        if (strcmp(type, "CREATE_GROUP") == 0)
        {
            handle_create_group(client, buffer);
        }
        else if (strcmp(type, "LIST_ALL_GROUPS") == 0)
        {
            handle_list_all_groups(client, buffer);
        }
        else if (strcmp(type, "JOIN_GROUP") == 0)
        {
            handle_join_group(client, buffer);
        }
        else if (strcmp(type, "INVITE_TO_GROUP") == 0)
        {
            handle_invite_to_group(client, buffer);
        }
        else if (strcmp(type, "LEAVE_GROUP") == 0)
        {
            handle_leave_group(client, buffer);
        }
        else if (strcmp(type, "LIST_GROUP_MEMBERS") == 0)
        {
            handle_list_group_members(client, buffer);
        }
        else if (strcmp(type, "LIST_INVITATION") == 0)
        {
            handle_list_invitations(client, buffer);
        }
        else if (strcmp(type, "REMOVE_MEMBER") == 0)
        {
            handle_remove_member(client, buffer);
        }
        else if (strcmp(type, "APPROVAL") == 0)
        {
            handle_approval(client, buffer);
        }
        else if (strcmp(type, "JOIN_REQUEST_STATUS") == 0)
        {
            handle_join_request_status(client, buffer);
        }
        else if (strcmp(type, "JOIN_REQUEST_LIST") == 0)
        {
            handle_join_request_list(client, buffer);
        }
        // Folder operations
        else if (strcmp(type, "FOLDER_CONTENT") == 0)
        {
            handle_folder_content(client, buffer);
        }
        else if (strcmp(type, "CREATE_FOLDER") == 0)
        {
            handle_create_folder(client, buffer);
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
        // File operations
        else if (strcmp(type, "UPLOAD_FILE") == 0)
        {
            handle_upload_file(client, buffer);
        }
        else if (strcmp(type, "DOWNLOAD_FILE") == 0)
        {
            handle_download_file(client, buffer);
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
        // User operations
        else if (strcmp(type, "LOGIN") == 0)
        {
            handle_login(client, buffer);
        }
        else if (strcmp(type, "REGISTER") == 0)
        {
            handle_register(client, buffer);
        }
        
        json_object_put(parsed_json);
    }

    close(client->socket);
    free(client);
    return NULL;
}

void handle_login(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *username_obj, *password_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "userName", &username_obj);
    json_object_object_get_ex(payload, "password", &password_obj);

    const char *username = json_object_get_string(username_obj);
    const char *password = json_object_get_string(password_obj);

    pthread_mutex_lock(&users_mutex);
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char *)username;
    bind[0].buffer_length = strlen(username);

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (char *)password;
    bind[1].buffer_length = strlen(password);

    pthread_mutex_unlock(&users_mutex);
    bool success = db_login(username, password);
    printf("Login success: %d\n", success);
    if (success == 1)
    {
        strncpy(client->username, username, MAX_USERNAME - 1);
        client->is_logged_in = 1;
        send_response(client->socket, 200, "Login successful");
    }
    else
    {
        send_response(client->socket, 401, "Invalid username or password");
    }

    json_object_put(parsed_json);
}

void handle_register(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *username_obj, *password_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "userName", &username_obj);
    json_object_object_get_ex(payload, "password", &password_obj);

    const char *username = json_object_get_string(username_obj);
    const char *password = json_object_get_string(password_obj);

    pthread_mutex_lock(&users_mutex);

    if (db_create_user(username, password))
    {
        strncpy(client->username, username, MAX_USERNAME - 1);
        client->is_logged_in = 1;
        send_response(client->socket, 201, "Registration successful");
    }
    else
    {
        send_response(client->socket, 409, "User already exists");
    }
    pthread_mutex_unlock(&users_mutex);

    json_object_put(parsed_json);
}

void send_response(int socket, int code, const char *message)
{
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "{\"responseCode\": %d, \"message\": \"%s\"}", code, message);
    send(socket, response, strlen(response), 0);
}