#include <arpa/inet.h>
#include <dirent.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "./server/system/system_access.h"
#include "./utils/helper.h"
#include "./utils/structs.h"
#include "utils/config.h"

#define PORT 5555
#define BUFFER_SIZE 8192
#define MAX_COMMAND_LENGTH 1024
#define MAX_USERNAME 32
#define MAX_PASSWORD 32
#define MAX_FOLDER_NAME 32
#define MAX_PATH_LENGTH 4096

// Function prototypes
void print_usage(void);
char *clean_file_path(const char *path);
void handle_login(int sock, const char *username, const char *password,
                  int *is_logged_in, char *current_user, struct json_object *j);
void handle_register(int sock, const char *username, const char *password,
                     int *is_logged_in, char *current_user,
                     struct json_object *j);
// void handle_folder_content(int sock, const char *group_name, const char
// *folder_name, struct json_object *j); void print_table_files(struct
// json_object *folder_content_array, const char *folder_name, const char
// *group_name);
void handle_folder_create_client(int sock, const char *folder_path,
                                 const char *folder_name,
                                 struct json_object *j);
void handle_folder_rename_client(int sock, const char *folder_path,
                                 const char *new_name, struct json_object *j);
void handle_folder_copy_client(int sock, const char *from_folder,
                               const char *to_folder, struct json_object *j);
void handle_folder_move_client(int sock, const char *from_folder,
                               const char *to_folder, struct json_object *j);
void handle_folder_delete_client(int sock, const char *folder_path,
                                 struct json_object *j);
void handle_folder_search_client(int sock, const char *search_term,
                                 struct json_object *j);
void handle_folder_download_client(int sock, const char *folder_path,
                                   const char *folder_owner,
                                   const char *des_folder_path,
                                   struct json_object *j);
void handle_folder_upload_client(int sock, const char *folder_path,
                                 const char *folder_name,
                                 struct json_object *j);
void handle_file_upload_client(int sock, const char *folder_path,
                               const char *file_path, struct json_object *j);
void handle_file_download_client(int sock, const char *file_path,
                                 const char *file_owner, struct json_object *j);
void handle_file_rename_client(int sock, const char *file_path,
                               const char *new_name, struct json_object *j);
void handle_file_copy_client(int sock, const char *file_path,
                             const char *to_folder, struct json_object *j);
void handle_file_move_client(int sock, const char *file_path,
                             const char *to_folder, struct json_object *j);
void handle_file_delete_client(int sock, const char *file_path,
                               struct json_object *j);
void handle_file_search_client(int sock, const char *file_name,
                               struct json_object *jobj);
void handle_logout(int sock, int *is_logged_in, char *current_user);
void handle_file_search_client(int sock, const char *search_term,
                               struct json_object *j);
void send_folder(int sock, const char *upload_folder_path,
                 const char *des_folder_path);
const char *get_filename(const char *path);

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
        command[strcspn(command, "\n")] = 0;  // Remove newline
        if (strcmp(command, "LOGOUT") == 0)
        {
            if (is_logged_in)
            {
                handle_logout(sock, &is_logged_in, current_user);
            }
            else
            {
                printf("You are not logged in!\n");
            }
            continue;
        }
        else if (strcmp(command, "HELP") == 0)
        {
            print_usage();
            continue;
        }

        struct json_object *jobj = json_object_new_object();

        if (strcmp(command, "LOGIN") == 0)
        {
            char username[MAX_USERNAME], password[MAX_PASSWORD];
            printf("Enter username: ");
            scanf("%s", username);
            printf("Enter password: ");
            scanf("%s", password);
            getchar();  // Consume newline

            handle_login(sock, username, password, &is_logged_in, current_user,
                         jobj);
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
            getchar();  // Consume newline

            handle_register(sock, username, password, &is_logged_in,
                            current_user, jobj);
        }
        // TODO: add FOLDER_CONTENT handler
        // else if (strcmp(command, "FOLDER_CONTENT") == 0)
        // {
        //     if (!is_logged_in)
        //     {
        //         printf("You do not have permission to see folder content.
        //         Please log in.\n"); continue;
        //     }

        //     char folder_name[MAX_FOLDER_NAME];
        //     getchar(); // Consume newline
        //     printf("Enter folder name: ");
        //     scanf("%s", folder_name);
        //     getchar(); // Consume newline

        //     handle_folder_content(sock, group_name, folder_name, jobj);
        // }
        else if (strcmp(command, "FOLDER_CREATE") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to create a folder. Please log "
                    "in!\n");
                continue;
            }

            char *folder_path = NULL;
            char folder_name[MAX_FOLDER_NAME];
            char temp_buffer[MAX_PATH_LENGTH];

            printf("Enter folder path: ");
            if (fgets(temp_buffer, MAX_PATH_LENGTH, stdin) != NULL)
            {
                // Remove trailing newline if present
                temp_buffer[strcspn(temp_buffer, "\n")] = 0;

                // If input is not empty, allocate and copy
                if (strlen(temp_buffer) > 0)
                {
                    folder_path = strdup(temp_buffer);
                }
            }
            // getchar(); // Consume newline
            printf("Enter folder name: ");
            scanf("%s", folder_name);
            getchar();  // Consume newline

            handle_folder_create_client(sock, folder_path, folder_name, jobj);
        }
        else if (strcmp(command, "FOLDER_RENAME") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to rename a folder. Please log "
                    "in!\n");
                continue;
            }

            char folder_path[MAX_PATH_LENGTH];
            char new_name[MAX_FOLDER_NAME];
            printf("Enter folder path: ");
            scanf("%s", folder_path);
            getchar();  // Consume newline
            printf("Enter new name: ");
            scanf("%s", new_name);
            getchar();  // Consume newline

            handle_folder_rename_client(sock, folder_path, new_name, jobj);
        }
        else if (strcmp(command, "FOLDER_COPY") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to copy a folder. Please log "
                    "in!\n");
                continue;
            }

            char from_folder[MAX_PATH_LENGTH];
            char to_folder[MAX_PATH_LENGTH];
            printf("From folder: ");
            scanf("%s", from_folder);
            getchar();  // Consume newline
            printf("To folder: ");
            scanf("%s", to_folder);
            getchar();  // Consume newline

            handle_folder_copy_client(sock, from_folder, to_folder, jobj);
        }
        else if (strcmp(command, "FOLDER_MOVE") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to move a folder. Please log "
                    "in!\n");
                continue;
            }

            char from_folder[MAX_PATH_LENGTH];
            char to_folder[MAX_PATH_LENGTH];
            printf("From folder: ");
            scanf("%s", from_folder);
            getchar();  // Consume newline
            printf("To folder: ");
            scanf("%s", to_folder);
            getchar();  // Consume newline

            handle_folder_move_client(sock, from_folder, to_folder, jobj);
        }
        else if (strcmp(command, "FOLDER_DELETE") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to delete a folder. Please log "
                    "in!\n");
                continue;
            }

            char folder_path[MAX_PATH_LENGTH];
            printf("Enter folder path: ");
            scanf("%s", folder_path);
            getchar();  // Consume newline

            handle_folder_delete_client(sock, folder_path, jobj);
        }
        else if (strcmp(command, "FOLDER_SEARCH") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to search folders. Please log "
                    "in!\n");
                continue;
            }

            char search_term[MAX_FOLDER_NAME];
            printf("Enter search term: ");
            scanf("%s", search_term);
            getchar();  // Consume newline

            handle_folder_search_client(sock, search_term, jobj);
        }
        else if (strcmp(command, "FOLDER_DOWNLOAD") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to download a folder. Please "
                    "log in!\n");
                continue;
            }

            char folder_path[MAX_PATH_LENGTH];
            char folder_owner[MAX_USERNAME];
            char des_folder_path[MAX_PATH_LENGTH];

            printf("Enter folder path: ");
            fgets_not_newline(folder_path, sizeof(folder_path));
            printf("Enter folder owner: ");
            fgets_not_newline(folder_owner, sizeof(folder_owner));
            printf("Enter destination folder path: ");
            scanf("%s", des_folder_path);
            getchar();  // Consume newline
            if (!is_folder_exist(des_folder_path))
            {
                printf("Folder not found: %s\n", des_folder_path);
                continue;
            }
            printf("Folder path: %s\n", folder_path);

            handle_folder_download_client(sock, folder_path, folder_owner,
                                          des_folder_path, jobj);
        }
        else if (strcmp(command, "FOLDER_UPLOAD") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to upload a folder. Please log "
                    "in!\n");
                continue;
            }

            char des_folder_path[MAX_PATH_LENGTH];
            char folder_path[MAX_FOLDER_NAME];
            printf("Enter destination folder path: ");
            fgets_not_newline(des_folder_path, sizeof(des_folder_path));
            printf("Enter folder path: ");
            scanf("%s", folder_path);
            getchar();  // Consume newline

            handle_folder_upload_client(sock, des_folder_path, folder_path,
                                        jobj);
        }
        else if (strcmp(command, "FILE_UPLOAD") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to upload a file. Please log "
                    "in.\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar();  // Consume newline

            char *folder_path = NULL;
            char folder_name[MAX_FOLDER_NAME];
            char temp_buffer[MAX_PATH_LENGTH];

            printf("Upload at folder: ");
            scanf("%s", temp_buffer);
            getchar();
            if (strlen(temp_buffer) > 0)
            {
                // Remove trailing newline if present
                temp_buffer[strcspn(temp_buffer, "\n")] = 0;

                // If input is not empty, allocate and copy
                if (strlen(temp_buffer) > 0)
                {
                    folder_path = strdup(temp_buffer);
                    printf("%s\n", folder_path);
                }
            }
            else
            {
                folder_path = NULL;
            }  // Consume newline

            handle_file_upload_client(sock, folder_path, file_path, jobj);
        }
        else if (strcmp(command, "FILE_DOWNLOAD") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to download a file. Please log "
                    "in.\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            char file_owner[MAX_USERNAME];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar();  // Consume newline
            printf("Enter file owner: ");
            scanf("%s", file_owner);
            getchar();  // Consume newline

            handle_file_download_client(sock, file_path, file_owner, jobj);
        }
        else if (strcmp(command, "FILE_RENAME") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to rename a file. Please log "
                    "in!\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            char new_name[MAX_COMMAND_LENGTH];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar();  // Consume newline
            printf("Enter new name: ");
            scanf("%s", new_name);
            getchar();  // Consume newline

            handle_file_rename_client(sock, file_path, new_name, jobj);
        }
        else if (strcmp(command, "FILE_COPY") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to copy a file. Please log "
                    "in!\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            char to_folder[MAX_PATH_LENGTH];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar();  // Consume newline
            printf("To folder: ");
            scanf("%s", to_folder);
            getchar();  // Consume newline

            handle_file_copy_client(sock, file_path, to_folder, jobj);
        }
        else if (strcmp(command, "FILE_MOVE") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to move a file. Please log "
                    "in!\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            char to_folder[MAX_PATH_LENGTH];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar();  // Consume newline
            printf("To folder: ");
            scanf("%s", to_folder);
            getchar();  // Consume newline

            handle_file_move_client(sock, file_path, to_folder, jobj);
        }
        else if (strcmp(command, "FILE_DELETE") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to delete a file. Please log "
                    "in!\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar();  // Consume newline

            handle_file_delete_client(sock, file_path, jobj);
        }
        else if (strcmp(command, "FILE_SEARCH") == 0)
        {
            if (!is_logged_in)
            {
                printf(
                    "You do not have permission to search files. Please log "
                    "in!\n");
                continue;
            }

            char search_term[MAX_COMMAND_LENGTH];
            printf("Enter search term: ");
            scanf("%s", search_term);
            getchar();  // Consume newline

            handle_file_search_client(sock, search_term, jobj);
        }
        else
        {
            printf("Command not recognized!\n");
            printf("Use command \"HELP\" to show the usage!\n");
            json_object_put(jobj);  // Free the JSON object
        }
    }

    close(sock);
    return 0;
}

void handle_login(int sock, const char *username, const char *password,
                  int *is_logged_in, char *current_user,
                  struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "userName",
                           json_object_new_string(username));
    json_object_object_add(jpayload, "password",
                           json_object_new_string(password));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("LOGIN"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response and update login status
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);
    json_object_object_get_ex(parsed_json, "payload", &payload);

    print_message_oneline(json_object_get_int(response_code), payload);

    if (json_object_get_int(response_code) == 200)
    {
        *is_logged_in = 1;
        strncpy(current_user, username, MAX_USERNAME - 1);
    }

    json_object_put(parsed_json);  // Free the JSON object
}

void handle_register(int sock, const char *username, const char *password,
                     int *is_logged_in, char *current_user,
                     struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "userName",
                           json_object_new_string(username));
    json_object_object_add(jpayload, "password",
                           json_object_new_string(password));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("REGISTER"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);
    json_object_object_get_ex(parsed_json, "payload", &payload);

    print_message_oneline(json_object_get_int(response_code), payload);

    if (json_object_get_int(response_code) == 201)
    {
        *is_logged_in = 1;
        strncpy(current_user, username, MAX_USERNAME - 1);
    }

    json_object_put(parsed_json);  // Free the JSON object
}

void handle_logout(int sock, int *is_logged_in, char *current_user)
{
    struct json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("LOGOUT"));
    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];
    recv(sock, buffer, BUFFER_SIZE, 0);

    handle_print_payload_response(buffer, print_message_oneline);

    *is_logged_in = 0;
    strcpy(current_user, "");
}

// TODO: handle folder content
// void handle_folder_content(int sock, const char *group_name, const char
// *folder_name, struct json_object *jobj)
// {
//     struct json_object *jpayload = json_object_new_object();

//     json_object_object_add(jpayload, "groupName",
//     json_object_new_string(group_name)); json_object_object_add(jpayload,
//     "folderName", json_object_new_string(folder_name));
//     json_object_object_add(jobj, "messageType",
//     json_object_new_string("FOLDER_CONTENT")); json_object_object_add(jobj,
//     "payload", jpayload);

//     const char *request = json_object_to_json_string(jobj);
//     send(sock, request, strlen(request), 0);

//     json_object_put(jobj); // Free the JSON object

//     char buffer[BUFFER_SIZE];

//     // Check response
//     recv(sock, buffer, BUFFER_SIZE, 0);

//     struct json_object *parsed_json;
//     struct json_object *response_code;
//     struct json_object *payload;
//     struct json_object *folder_content_array;

//     parsed_json = json_tokener_parse(buffer);
//     json_object_object_get_ex(parsed_json, "responseCode", &response_code);

//     int response_code_int = json_object_get_int(response_code);
//     switch (response_code_int)
//     {
//     case 200:
//         json_object_object_get_ex(parsed_json, "payload", &payload);
//         json_object_object_get_ex(payload, "folderContents",
//         &folder_content_array); print_table_files(folder_content_array,
//         folder_name, group_name); break;
//     case 404:
//         printf("Folder does not exist!\n");
//         break;
//     default:
//         printf("You are not a member in group!\n");
//         break;
//     }

//     json_object_put(parsed_json); // Free the JSON object
// }

// void print_table_files(struct json_object *folder_content_array, const char
// *folder_name, const char *group_name)
// {
//     printf("Contents of folder %s in group %s:\n", folder_name, group_name);
//     int array_len = json_object_array_length(folder_content_array);
//     for (int i = 0; i < array_len; i++)
//     {
//         struct json_object *file =
//         json_object_array_get_idx(folder_content_array, i); printf("%s\n",
//         json_object_get_string(file));
//     }
// }

void handle_folder_create_client(int sock, const char *folder_path,
                                 const char *folder_name,
                                 struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();
    if (folder_path != NULL)
        json_object_object_add(jpayload, "folderPath",
                               json_object_new_string(folder_path));
    else
        json_object_object_add(jpayload, "folderPath",
                               json_object_new_string(""));
    json_object_object_add(jpayload, "folderName",
                           json_object_new_string(folder_name));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FOLDER_CREATE"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    handle_print_payload_response(buffer, print_message_oneline);
}

void handle_folder_download_client(int sock, const char *folder_path,
                                   const char *folder_owner,
                                   const char *des_folder_path,
                                   struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "folderPath",
                           json_object_new_string(folder_path));
    json_object_object_add(jpayload, "folderOwner",
                           json_object_new_string(folder_owner));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FOLDER_DOWNLOAD"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);
    json_object_object_get_ex(parsed_json, "payload", &payload);

    int response_code_int = json_object_get_int(response_code);
    print_message_oneline(response_code_int, payload);

    if (response_code_int == 200)
    {
        struct json_object *file_size_obj;
        struct json_object *zip_folder_name_obj;

        json_object_object_get_ex(payload, "fileSize", &file_size_obj);
        json_object_object_get_ex(payload, "zipFoldername",
                                  &zip_folder_name_obj);

        long file_size = json_object_get_int64(file_size_obj);
        const char *zip_folder_name =
            json_object_get_string(zip_folder_name_obj);

        char zip_folder_path[MAX_PATH_LENGTH];
        snprintf(zip_folder_path, sizeof(zip_folder_path), "%s/%s",
                 des_folder_path, zip_folder_name);

        FILE *f = fopen(zip_folder_path, "wb");
        if (!f)
        {
            printf("Failed to open destination file!\n");
            return;
        }

        send(sock, "OK", strlen("OK"), 0);

        receive_write_file(sock, file_size, f);
        printf("\nDownload succeed!\n");
    }

    json_object_put(parsed_json);  // Free the JSON object
}

void handle_folder_upload_client(int sock, const char *des_folder_path,
                                 const char *upload_folder_path,
                                 struct json_object *jobj)
{
    if (!is_folder_exist(upload_folder_path))
    {
        printf("Folder does not exist: %s\n", upload_folder_path);
        return;
    }

    int file_count = count_files_in_folder(upload_folder_path);

    const char *folder_name = get_folder_name(upload_folder_path);
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "folderName",
                           json_object_new_string(folder_name));
    json_object_object_add(jpayload, "folderPath",
                           json_object_new_string(des_folder_path));
    json_object_object_add(jpayload, "fileCount",
                           json_object_new_int(file_count));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FOLDER_UPLOAD"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;
    struct json_object *valid_folder_name_obj;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);
    json_object_object_get_ex(parsed_json, "payload", &payload);
    int response_code_int = json_object_get_int(response_code);

    print_message_oneline(response_code_int, payload);

    if (response_code_int == 200)
    {
        printf("Upload start. Please wait!\n");

        json_object_object_get_ex(payload, "validFolderName",
                                  &valid_folder_name_obj);
        const char *valid_folder_name =
            json_object_get_string(valid_folder_name_obj);
        char valid_des_folder_path[MAX_PATH_LENGTH];

        if (strlen(des_folder_path) == 0)
        {
            snprintf(valid_des_folder_path, sizeof(valid_des_folder_path), "%s",
                     valid_folder_name);
        }
        else
        {
            snprintf(valid_des_folder_path, sizeof(valid_des_folder_path),
                     "%s/%s", des_folder_path, valid_folder_name);
        }

        send_folder(sock, upload_folder_path, valid_des_folder_path);

        char buffer[BUFFER_SIZE];
        recv(sock, buffer, BUFFER_SIZE, 0);
        handle_print_payload_response(buffer, print_message_oneline);
    }

    json_object_put(parsed_json);  // Free the JSON object
}

void send_folder(int sock, const char *upload_folder_path,
                 const char *des_folder_path)
{
    DIR *dir = opendir(upload_folder_path);

    struct dirent *entry;
    char upload_path[MAX_PATH_LENGTH];
    char des_path[MAX_PATH_LENGTH];

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }
            snprintf(upload_path, sizeof(upload_path), "%s/%s",
                     upload_folder_path, entry->d_name);
            snprintf(des_path, sizeof(des_path), "%s/%s", des_folder_path,
                     entry->d_name);
            send_folder(sock, upload_path, des_path);
        }
        else
        {
            snprintf(upload_path, sizeof(upload_path), "%s/%s",
                     upload_folder_path, entry->d_name);
            snprintf(des_path, sizeof(des_path), "%s/%s", des_folder_path,
                     entry->d_name);

            FILE *file = fopen(upload_path, "rb");
            if (!file)
            {
                printf("Failed to open file: %s\n", upload_path);
                send(sock, CLIENT_ERROR, strlen(CLIENT_ERROR), 0);
                continue;
            }

            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);

            struct json_object *jpayload = json_object_new_object();
            json_object_object_add(jpayload, "filePath",
                                   json_object_new_string(des_path));
            json_object_object_add(jpayload, "fileSize",
                                   json_object_new_int64(file_size));
            const char *jpayload_str = json_object_to_json_string(jpayload);

            send(sock, jpayload_str, strlen(jpayload_str), 0);

            char buffer[BUFFER_SIZE];
            recv(sock, buffer, BUFFER_SIZE, 0);

            struct json_object *parsed_json = json_tokener_parse(buffer);
            struct json_object *response_code_obj;
            struct json_object *payload_obj;
            json_object_object_get_ex(parsed_json, "responseCode",
                                      &response_code_obj);
            json_object_object_get_ex(parsed_json, "payload", &payload_obj);

            int response_code = json_object_get_int(response_code_obj);
            if (response_code == 200)
            {
                printf("Uploading file: %s\n", upload_path);
                read_send_file(sock, file_size, file);
            }
            else
                print_message_oneline(response_code, payload_obj);

            json_object_put(parsed_json);
            json_object_put(jpayload);
        }
    }

    closedir(dir);
}

void handle_folder_rename_client(int sock, const char *folder_path,
                                 const char *new_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "folderPath",
                           json_object_new_string(folder_path));
    json_object_object_add(jpayload, "newFolderName",
                           json_object_new_string(new_name));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FOLDER_RENAME"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    handle_print_payload_response(buffer, print_message_oneline);
}

void handle_folder_copy_client(int sock, const char *from_folder,
                               const char *to_folder, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "fromFolder",
                           json_object_new_string(from_folder));
    json_object_object_add(jpayload, "toFolder",
                           json_object_new_string(to_folder));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FOLDER_COPY"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    handle_print_payload_response(buffer, print_message_oneline);
}

void handle_folder_move_client(int sock, const char *from_folder,
                               const char *to_folder, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "fromFolder",
                           json_object_new_string(from_folder));
    json_object_object_add(jpayload, "toFolder",
                           json_object_new_string(to_folder));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FOLDER_MOVE"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    handle_print_payload_response(buffer, print_message_oneline);
}

void handle_folder_delete_client(int sock, const char *folder_path,
                                 struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "folderPath",
                           json_object_new_string(folder_path));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FOLDER_DELETE"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    handle_print_payload_response(buffer, print_message_oneline);
}

void handle_folder_search_client(int sock, const char *folder_name,
                                 struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "folderName",
                           json_object_new_string(folder_name));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FOLDER_SEARCH"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    // Check response
    char *response = handle_response_chunk(sock, BUFFER_SIZE);

    struct json_object *parsed_json = json_tokener_parse(response);
    struct json_object *response_code;
    struct json_object *payload;
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);
    json_object_object_get_ex(parsed_json, "payload", &payload);
    printf("responseCode: %d\n", json_object_get_int(response_code));
    struct json_object *folder_content_array =
        json_object_object_get(payload, "folders");

    printf("%16s %6s %6s %s \n", "Folder Name", "Access", "Created At",
           "Folder Path");
    int array_size = json_object_array_length(folder_content_array);
    for (int i = 0; i < array_size; i++)
    {
        struct json_object *folder =
            json_object_array_get_idx(folder_content_array, i);
        struct json_object *folder_name =
            json_object_object_get(folder, "folderName");
        struct json_object *access = json_object_object_get(folder, "access");
        struct json_object *created_at =
            json_object_object_get(folder, "createdAt");
        struct json_object *folder_path =
            json_object_object_get(folder, "folderPath");
        printf("%16s %6s %6s %9s \n", json_object_get_string(folder_name),
               json_object_get_string(access),
               json_object_get_string(created_at),
               json_object_get_string(folder_path));
    }

    json_object_put(parsed_json);
}

void handle_file_upload_client(int sock, const char *folder_path,
                               const char *file_path, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        printf("Source file path does not exist!!!\n");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (folder_path != NULL)
        json_object_object_add(jpayload, "folderPath",
                               json_object_new_string(folder_path));
    else
        json_object_object_add(jpayload, "folderPath",
                               json_object_new_string(""));
    json_object_object_add(jpayload, "fileName",
                           json_object_new_string(get_filename(file_path)));
    json_object_object_add(jpayload, "fileSize",
                           json_object_new_int64(file_size));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FILE_UPLOAD"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];
    int is_done = 0;
    do
    {
        recv(sock, buffer, BUFFER_SIZE, 0);

        struct json_object *parsed_json;
        struct json_object *response_code;
        struct json_object *payload;
        parsed_json = json_tokener_parse(buffer);
        json_object_object_get_ex(parsed_json, "responseCode", &response_code);
        json_object_object_get_ex(parsed_json, "payload", &payload);
        int response_code_int = json_object_get_int(response_code);
        print_message_oneline(response_code_int, payload);

        if (response_code_int == 409 || response_code_int == 400)
        {
            printf("File already exists. Do you want to overwrite it? (Y/N)\n");
            char answer[2];
            fgets(answer, sizeof(answer), stdin);
            strtok(answer, "\n");
            if (strlen(answer) == 0)
            {
                answer[0] = 'Y';
            }
            struct json_object *tmp = json_object_new_object();
            json_object_object_add(tmp, "answer",
                                   json_object_new_string(answer));
            const char *request = json_object_to_json_string(tmp);
            send(sock, request, strlen(request), 0);
            json_object_put(tmp);
            if (strcmp(answer, "N") == 0 || strcmp(answer, "n") == 0)
            {
                json_object_put(parsed_json);
                return;
            }
        }
        if (response_code_int == 500)
        {
            printf("Failed to create file!\n");
            json_object_put(parsed_json);
            return;
        }

        if (response_code_int == 404)
        {
            printf("Folder not found!\n");
            json_object_put(parsed_json);
            return;
        }

        if (response_code_int == 200)
        {
            printf("Upload start. Please wait!\n");
            int data;
            long byte_send = 0;
            while ((data = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
            {
                send(sock, buffer, data, 0);
                byte_send += data;
                printf("Progress: %ld/%ld bytes\r", byte_send, file_size);
                fflush(stdout);
            }
            fclose(file);
            is_done = 1;
            printf("\nUploaded!\n");
        }
        json_object_put(parsed_json);  // Free the JSON object
    } while (!is_done);
}

void handle_file_download_client(int sock, const char *file_path,
                                 const char *file_owner,
                                 struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "filePath",
                           json_object_new_string(file_path));
    json_object_object_add(jpayload, "fileOwner",
                           json_object_new_string(file_owner));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FILE_DOWNLOAD"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);
    json_object_object_get_ex(parsed_json, "payload", &payload);

    int response_code_int = json_object_get_int(response_code);
    print_message_oneline(response_code_int, payload);

    if (response_code_int == 200)
    {
        struct json_object *file_size_obj;
        json_object_object_get_ex(payload, "fileSize", &file_size_obj);
        long file_size = json_object_get_int64(file_size_obj);
        json_object_put(parsed_json);  // Free the JSON object

        if (file_size != 0)
        {
            char path[MAX_PATH_LENGTH];
            printf("Input destination path: ");
            scanf("%s", path);
            getchar();  // Consume newline

            struct stat st = {0};
            if (stat(path, &st) == -1)
            {
                mkdir(path, 0777);
            }

            char des_path[MAX_PATH_LENGTH];
            const char *file_name = get_filename(file_path);
            snprintf(des_path, sizeof(des_path) + 1, "%s/%s", path, file_name);

            FILE *f = fopen(des_path, "wb");
            if (!f)
            {
                printf("Failed to open destination file!\n");
                return;
            }

            long byte_readed = 0;
            int bytes_read;
            while (byte_readed < file_size)
            {
                bytes_read = recv(sock, buffer, BUFFER_SIZE, 0);
                fwrite(buffer, 1, bytes_read, f);
                byte_readed += bytes_read;
                printf("Progress: %ld/%ld bytes\r", byte_readed, file_size);
                fflush(stdout);
            }
            fclose(f);
            printf("\nDownload succeed!\n");
        }
    }
}

void handle_file_rename_client(int sock, const char *file_path,
                               const char *new_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "filePath",
                           json_object_new_string(file_path));
    json_object_object_add(jpayload, "newFileName",
                           json_object_new_string(new_name));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FILE_RENAME"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    handle_print_payload_response(buffer, print_message_oneline);
}

void handle_file_copy_client(int sock, const char *file_path,
                             const char *to_folder, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "filePath",
                           json_object_new_string(file_path));
    json_object_object_add(jpayload, "toFolder",
                           json_object_new_string(to_folder));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FILE_COPY"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    handle_print_payload_response(buffer, print_message_oneline);
}

void handle_file_move_client(int sock, const char *file_path,
                             const char *to_folder, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "filePath",
                           json_object_new_string(file_path));
    json_object_object_add(jpayload, "toFolder",
                           json_object_new_string(to_folder));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FILE_MOVE"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    handle_print_payload_response(buffer, print_message_oneline);
}

void handle_file_delete_client(int sock, const char *file_path,
                               struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "filePath",
                           json_object_new_string(file_path));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FILE_DELETE"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj);  // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    handle_print_payload_response(buffer, print_message_oneline);
}

void handle_file_search_client(int sock, const char *file_name,
                               struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "fileName",
                           json_object_new_string(file_name));
    json_object_object_add(jobj, "messageType",
                           json_object_new_string("FILE_SEARCH"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    char *response = handle_response_chunk(sock, BUFFER_SIZE);
    // TODO: Update print found files
    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;
    struct json_object *files;
    parsed_json = json_tokener_parse(response);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "files", &files);
    int response_code_int = json_object_get_int(response_code);
    printf("Response code: %d\n", response_code_int);
    print_file_table(files);
}

char *clean_file_path(const char *path)
{
    char *result = strdup(path);  // Make a copy of the string
    char *src = result;
    char *dst = result;

    while (*src)
    {
        if (src[0] == '\\' && src[1] == '/')
        {
            src++;  // Skip the backslash
        }
        *dst++ = *src++;
    }
    *dst = '\0';  // Null terminate

    return result;
}

void print_usage(void)
{
    printf("Commands:\n");
    printf("LOGIN - Log in to the system\n");
    printf("REGISTER - Register a new user\n");
    printf("LOGOUT - Log out of the system\n");
    printf("FOLDER_CONTENT - List all files in a folder\n");
    printf("FOLDER_CREATE - Create a new folder\n");
    printf("FOLDER_RENAME - Rename a folder\n");
    printf("FOLDER_COPY - Copy a folder to another folder\n");
    printf("FOLDER_MOVE - Move a folder to another folder\n");
    printf("FOLDER_DELETE - Delete a folder\n");
    printf("FOLDER_SEARCH - Search for a folder\n");
    printf("FOLDER_DOWNLOAD - Download a folder\n");
    printf("FOLDER_UPLOAD - Upload a folder\n");
    printf("FILE_UPLOAD - Upload a file to a folder\n");
    printf("FILE_DOWNLOAD - Download a file from a folder\n");
    printf("FILE_RENAME - Rename a file\n");
    printf("FILE_COPY - Copy a file to another folder\n");
    printf("FILE_MOVE - Move a file to another folder\n");
    printf("FILE_DELETE - Delete a file\n");
    printf("FILE_SEARCH - Search for a file\n");
    printf("HELP - Show usage\n");
}
