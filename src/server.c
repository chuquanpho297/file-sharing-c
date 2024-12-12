#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <json-c/json.h>
#include "db_utils.h"
#include "server/file_handler.h"

#define PORT 5555
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 10
#define MAX_USERNAME 32
#define MAX_PASSWORD 32

// Database credentials
const char *host = "127.0.0.1";
const char *user = "root";
const char *password = "123456";
const char *db_name = "file_sharing";
const unsigned int port = 3306;

typedef struct
{
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} user_t;


MYSQL *conn;

int user_count = 0;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void *handle_client(void *arg);
void handle_login(client_t *client, const char *buffer);
void handle_register(client_t *client, const char *buffer);
void send_response(int socket, int code, const char *message);

int main()
{
    conn = db_connecting(host, user, password, db_name, port);
    if (conn == NULL)
    {
        fprintf(stderr, "db_connect failed: %s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }
    printf("Database connection successful.\n");
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
        // send_response(client->socket, 200, "Connected to server");

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

        if (strcmp(type, "UPLOAD_FILE") == 0) {
            handle_upload_file(client, buffer);
        }
        else if (strcmp(type, "DOWNLOAD_FILE") == 0) {
            handle_download_file(client, buffer);
        }
        else if (strcmp(type, "FILE_RENAME") == 0) {
            handle_file_rename(client, buffer);
        }
        else if (strcmp(type, "FILE_COPY") == 0) {
            handle_file_copy(client, buffer);
        }
        else if (strcmp(type, "FILE_MOVE") == 0) {
            handle_file_move(client, buffer);
        }
        else if (strcmp(type, "FILE_DELETE") == 0) {
            handle_file_delete(client, buffer);
        }
        else if (strcmp(type, "LOGIN") == 0)
        {
            handle_login(client, buffer);
        }
        else if (strcmp(type, "REGISTER") == 0)
        {
            handle_register(client, buffer);
        }
        else if (strcmp(type, "EXIT") == 0)
        {
            break;
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

    const char *query = "SELECT Login(?, ?) AS Success;";
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt)
    {
        fprintf(stderr, "mysql_stmt_init failed: %s\n", mysql_error(conn));
        send_response(client->socket, 500, "Internal server error");
        mysql_close(conn);
        return;
    }

    if (mysql_stmt_prepare(stmt, query, strlen(query)))
    {
        fprintf(stderr, "mysql_stmt_prepare failed: %s\n", mysql_stmt_error(stmt));
        send_response(client->socket, 500, "Internal server error");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return;
    }

    pthread_mutex_lock(&users_mutex);
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char *)username;
    bind[0].buffer_length = strlen(username);

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (char *)password;
    bind[1].buffer_length = strlen(password);

    if (mysql_stmt_bind_param(stmt, bind))
    {
        fprintf(stderr, "mysql_stmt_bind_param failed: %s\n", mysql_stmt_error(stmt));
        send_response(client->socket, 500, "Internal server error");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return;
    }

    if (mysql_stmt_execute(stmt))
    {
        fprintf(stderr, "mysql_stmt_execute failed: %s\n", mysql_stmt_error(stmt));
        send_response(client->socket, 500, "Internal server error");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return;
    }

    MYSQL_BIND result_bind[1];
    memset(result_bind, 0, sizeof(result_bind));

    int success;
    result_bind[0].buffer_type = MYSQL_TYPE_LONG;
    result_bind[0].buffer = (char *)&success;
    result_bind[0].is_null = 0;
    result_bind[0].length = 0;

    if (mysql_stmt_bind_result(stmt, result_bind))
    {
        fprintf(stderr, "mysql_stmt_bind_result failed: %s\n", mysql_stmt_error(stmt));
        send_response(client->socket, 500, "Internal server error");
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return;
    }

    pthread_mutex_unlock(&users_mutex);

    if (mysql_stmt_fetch(stmt) == 0)
    {
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
    }
    else
    {
        send_response(client->socket, 500, "Internal server error");
    }
    mysql_stmt_close(stmt);
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

    char query[256];
    snprintf(query, 256, "Select InsertNewUser('%s', '%s') AS Success;", username, password);

    if (mysql_query(conn, query))
    {
        fprintf(stderr, "CALL REGISTER failed: %s\n", mysql_error(conn));
        send_response(client->socket, 500, "Internal server error");
        mysql_close(conn);
        return;
    }

    pthread_mutex_lock(&users_mutex);
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL)
    {
        fprintf(stderr, "mysql_store_result failed: %s\n", mysql_error(conn));
        send_response(client->socket, 500, "Internal server error");
        mysql_close(conn);
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row && strcmp(row[0], "1") == 0)
    {
        strncpy(client->username, username, MAX_USERNAME - 1);
        client->is_logged_in = 1;
        send_response(client->socket, 201, "Registration successful");
    }
    else
    {
        send_response(client->socket, 409, "User already exists");
    }

    mysql_free_result(result);
    pthread_mutex_unlock(&users_mutex);

    json_object_put(parsed_json);
}

void send_response(int socket, int code, const char *message)
{
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "{\"responseCode\": %d, \"message\": \"%s\"}", code, message);
    send(socket, response, strlen(response), 0);
}