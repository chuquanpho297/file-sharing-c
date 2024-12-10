#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <json-c/json.h>

#define PORT 5555
#define BUFFER_SIZE 4096
#define MAX_COMMAND_LENGTH 1024
#define MAX_USERNAME 32
#define MAX_PASSWORD 32

// Function prototypes
void print_usage(void);
void handle_login(int sock, const char *username, const char *password);
void handle_register(int sock, const char *username, const char *password);

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char command[MAX_COMMAND_LENGTH];
    int is_logged_in = 0;
    char current_user[MAX_USERNAME] = "";

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Receive initial connection response
    // recv(sock, buffer, BUFFER_SIZE, 0);
    // printf("%s\n", buffer);

    while (1)
    {
        if (!is_logged_in)
        {
            printf("#CLIENT> ");
        }
        else
        {
            printf("#%s> ", current_user);
        }

        // Get command from user
        fgets(command, MAX_COMMAND_LENGTH, stdin);
        command[strcspn(command, "\n")] = 0; // Remove newline

        if (strcmp(command, "LOGOUT") == 0)
        {
            if (is_logged_in)
            {
                printf("Logging out...\n");
                is_logged_in = 0;
            }
            else
            {
                printf("You are not logged in!\n");
            }
        }
        else if (strcmp(command, "LOGIN") == 0)
        {
            char username[MAX_USERNAME], password[MAX_PASSWORD];
            printf("Enter username: ");
            scanf("%s", username);
            printf("Enter password: ");
            scanf("%s", password);
            getchar(); // Consume newline

            handle_login(sock, username, password);

            // Check response and update login status
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("%s\n", buffer);

            struct json_object *parsed_json;
            struct json_object *response_code;

            parsed_json = json_tokener_parse(buffer);
            json_object_object_get_ex(parsed_json, "responseCode", &response_code);

            if (json_object_get_int(response_code) == 200)
            {
                is_logged_in = 1;
                strncpy(current_user, username, MAX_USERNAME - 1);
                printf("Login success!\n");
            }
            else
            {
                printf("Wrong Username or Password!\n");
            }

            json_object_put(parsed_json); // Free the JSON object
        }
        else if (strcmp(command, "REGISTER") == 0)
        {
            if (is_logged_in)
            {
                printf("You are already logged in!\n");
                continue;
            }

            char username[MAX_USERNAME], password[MAX_PASSWORD];
            printf("Enter username: ");
            scanf("%s", username);
            printf("Enter password: ");
            scanf("%s", password);
            getchar(); // Consume newline

            handle_register(sock, username, password);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("%s\n", buffer);

            struct json_object *parsed_json;
            struct json_object *response_code;

            parsed_json = json_tokener_parse(buffer);
            json_object_object_get_ex(parsed_json, "responseCode", &response_code);

            if (json_object_get_int(response_code) == 201)
            {
                is_logged_in = 1;
                strncpy(current_user, username, MAX_USERNAME - 1);
                printf("Registration successful! Hello %s\n", username);
            }
            else
            {
                printf("User already exists!\n");
            }

            json_object_put(parsed_json); // Free the JSON object
        }
        else if (strcmp(command, "HELP") == 0)
        {
            print_usage();
        }
        else
        {
            printf("Command not recognized!\n");
            printf("Use command \"HELP\" to show the usage!\n");
        }
    }

    close(sock);
    return 0;
}

void handle_login(int sock, const char *username, const char *password)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "userName", json_object_new_string(username));
    json_object_object_add(jpayload, "password", json_object_new_string(password));
    json_object_object_add(jobj, "messageType", json_object_new_string("LOGIN"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
}

void handle_register(int sock, const char *username, const char *password)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "userName", json_object_new_string(username));
    json_object_object_add(jpayload, "password", json_object_new_string(password));
    json_object_object_add(jobj, "messageType", json_object_new_string("REGISTER"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
}

void print_usage(void)
{
    printf("Commands:\n");
    printf("LOGIN - Log in to the system\n");
    printf("REGISTER - Register a new user\n");
    printf("LOGOUT - Log out of the system\n");
    printf("HELP - Show usage\n");
}