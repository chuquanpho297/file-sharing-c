#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <json-c/json.h>
#include <sys/stat.h>

#define PORT 5555
#define BUFFER_SIZE 4096
#define MAX_COMMAND_LENGTH 1024
#define MAX_USERNAME 32
#define MAX_PASSWORD 32
#define MAX_FOLDER_NAME 32
#define MAX_PATH_LENGTH 1024

// Function prototypes
void print_usage(void);
void handle_login(int sock, const char *username, const char *password, int *is_logged_in, char *current_user, struct json_object *j);
void handle_register(int sock, const char *username, const char *password, int *is_logged_in, char *current_user, struct json_object *j);
void handle_folder_content(int sock, const char *group_name, const char *folder_name, struct json_object *j);
void print_table_files(struct json_object *folder_content_array, const char *folder_name, const char *group_name);
void handle_create_folder(int sock, const char *folder_path, const char *folder_name, struct json_object *j);
void handle_folder_rename(int sock, const char *folder_path, const char *new_name, struct json_object *j);
void handle_folder_copy(int sock, const char *from_folder, const char *to_folder, struct json_object *j);
void handle_folder_move(int sock, const char *from_folder, const char *to_folder, struct json_object *j);
void handle_folder_delete(int sock, const char *folder_path, struct json_object *j);
void handle_search_folders(int sock, const char *search_term, struct json_object *j);
void handle_download_folder(int sock, const char *folder_path, const char *folder_owner, struct json_object *j);
void handle_upload_folder(int sock, const char *folder_path, const char *folder_name, struct json_object *j);
void handle_upload_file(int sock, const char *folder_path, const char *file_path, struct json_object *j);
void handle_download_file(int sock, const char *file_path, const char *file_owner, struct json_object *j);
void handle_file_rename(int sock, const char *file_path, const char *new_name, struct json_object *j);
void handle_file_copy(int sock, const char *file_path, const char *to_folder, struct json_object *j);
void handle_file_move(int sock, const char *file_path, const char *to_folder, struct json_object *j);
void handle_file_delete(int sock, const char *file_path, struct json_object *j);
void handle_exit(int sock, struct json_object *j);
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
        command[strcspn(command, "\n")] = 0; // Remove newline
        if (strcmp(command, "LOGOUT") == 0)
        {
            if (is_logged_in)
            {
                printf("Logging out...\n");
                is_logged_in = 0;
                strcpy(current_user, "");
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
        json_object_object_add(jobj, "username", json_object_new_string(current_user));

        if (strcmp(command, "LOGIN") == 0)
        {
            char username[MAX_USERNAME], password[MAX_PASSWORD];
            printf("Enter username: ");
            scanf("%s", username);
            printf("Enter password: ");
            scanf("%s", password);
            getchar(); // Consume newline

            handle_login(sock, username, password, &is_logged_in, current_user, jobj);
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

            handle_register(sock, username, password, &is_logged_in, current_user, jobj);
        }
        // else if (strcmp(command, "FOLDER_CONTENT") == 0)
        // {
        //     if (!is_logged_in)
        //     {
        //         printf("You do not have permission to see folder content. Please log in.\n");
        //         continue;
        //     }

        //     char folder_name[MAX_FOLDER_NAME];
        //     getchar(); // Consume newline
        //     printf("Enter folder name: ");
        //     scanf("%s", folder_name);
        //     getchar(); // Consume newline

        //     handle_folder_content(sock, group_name, folder_name, jobj);
        // }
        else if (strcmp(command, "CREATE_FOLDER") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to create a folder. Please log in!\n");
                continue;
            }

            char folder_path[MAX_PATH_LENGTH];
            char folder_name[MAX_FOLDER_NAME];
            printf("Enter folder path: ");
            scanf("%s", folder_path);
            getchar(); // Consume newline
            printf("Enter folder name: ");
            scanf("%s", folder_name);
            getchar(); // Consume newline

            handle_create_folder(sock, folder_path, folder_name, jobj);
        }
        else if (strcmp(command, "FOLDER_RENAME") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to rename a folder. Please log in!\n");
                continue;
            }

            char folder_path[MAX_PATH_LENGTH];
            char new_name[MAX_FOLDER_NAME];
            printf("Enter folder path: ");
            scanf("%s", folder_path);
            getchar(); // Consume newline
            printf("Enter new name: ");
            scanf("%s", new_name);
            getchar(); // Consume newline

            handle_folder_rename(sock, folder_path, new_name, jobj);
        }
        else if (strcmp(command, "FOLDER_COPY") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to copy a folder. Please log in!\n");
                continue;
            }

            char from_folder[MAX_PATH_LENGTH];
            char to_folder[MAX_PATH_LENGTH];
            printf("From folder: ");
            scanf("%s", from_folder);
            getchar(); // Consume newline
            printf("To folder: ");
            scanf("%s", to_folder);
            getchar(); // Consume newline

            handle_folder_copy(sock, from_folder, to_folder, jobj);
        }
        else if (strcmp(command, "FOLDER_MOVE") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to move a folder. Please log in!\n");
                continue;
            }

            char from_folder[MAX_PATH_LENGTH];
            char to_folder[MAX_PATH_LENGTH];
            printf("From folder: ");
            scanf("%s", from_folder);
            getchar(); // Consume newline
            printf("To folder: ");
            scanf("%s", to_folder);
            getchar(); // Consume newline

            handle_folder_move(sock, from_folder, to_folder, jobj);
        }
        else if (strcmp(command, "FOLDER_DELETE") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to delete a folder. Please log in!\n");
                continue;
            }

            char folder_path[MAX_PATH_LENGTH];
            printf("Enter folder path: ");
            scanf("%s", folder_path);
            getchar(); // Consume newline

            handle_folder_delete(sock, folder_path, jobj);
        }
        else if (strcmp(command, "FOLDER_SEARCH") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to search folders. Please log in!\n");
                continue;
            }

            char search_term[MAX_FOLDER_NAME];
            printf("Enter search term: ");
            scanf("%s", search_term);
            getchar(); // Consume newline

            handle_search_folders(sock, search_term, jobj);
        }
        else if (strcmp(command, "FOLDER_DOWNLOAD") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to download a folder. Please log in!\n");
                continue;
            }

            char folder_path[MAX_PATH_LENGTH];
            char folder_owner[MAX_USERNAME];
            printf("Enter folder path: ");
            scanf("%s", folder_path);
            getchar(); // Consume newline
            printf("Enter folder owner: ");
            scanf("%s", folder_owner);
            getchar(); // Consume newline

            handle_download_folder(sock, folder_path, folder_owner, jobj);
        }
        else if (strcmp(command, "FOLDER_UPLOAD") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to upload a folder. Please log in!\n");
                continue;
            }

            char folder_path[MAX_PATH_LENGTH];
            char folder_name[MAX_FOLDER_NAME];
            printf("Enter folder path: ");
            scanf("%s", folder_path);
            getchar(); // Consume newline
            printf("Enter folder name: ");
            scanf("%s", folder_name);
            getchar(); // Consume newline

            handle_upload_folder(sock, folder_path, folder_name, jobj);
        }
        else if (strcmp(command, "UPLOAD_FILE") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to upload a file. Please log in.\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar(); // Consume newline

            char folder_path[MAX_PATH_LENGTH];
            printf("Upload at folder: ");
            scanf("%s", folder_path);
            getchar(); // Consume newline

            handle_upload_file(sock, folder_path, file_path, jobj);
        }
        else if (strcmp(command, "DOWNLOAD_FILE") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to download a file. Please log in.\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            char file_owner[MAX_USERNAME];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar(); // Consume newline
            printf("Enter file owner: ");
            scanf("%s", file_owner);
            getchar(); // Consume newline

            handle_download_file(sock, file_path, file_owner, jobj);
        }
        else if (strcmp(command, "FILE_RENAME") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to rename a file. Please log in!\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            char new_name[MAX_COMMAND_LENGTH];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar(); // Consume newline
            printf("Enter new name: ");
            scanf("%s", new_name);
            getchar(); // Consume newline

            handle_file_rename(sock, file_path, new_name, jobj);
        }
        else if (strcmp(command, "FILE_COPY") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to copy a file. Please log in!\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            char to_folder[MAX_PATH_LENGTH];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar(); // Consume newline
            printf("To folder: ");
            scanf("%s", to_folder);
            getchar(); // Consume newline

            handle_file_copy(sock, file_path, to_folder, jobj);
        }
        else if (strcmp(command, "FILE_MOVE") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to move a file. Please log in!\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            char to_folder[MAX_PATH_LENGTH];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar(); // Consume newline
            printf("To folder: ");
            scanf("%s", to_folder);
            getchar(); // Consume newline

            handle_file_move(sock, file_path, to_folder, jobj);
        }
        else if (strcmp(command, "FILE_DELETE") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to delete a file. Please log in!\n");
                continue;
            }

            char file_path[MAX_PATH_LENGTH];
            printf("Enter file path: ");
            scanf("%s", file_path);
            getchar(); // Consume newline

            handle_file_delete(sock, file_path, jobj);
        }
        else if (strcmp(command, "FILE_SEARCH") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to search files. Please log in!\n");
                continue;
            }

            char search_term[MAX_COMMAND_LENGTH];
            printf("Enter search term: ");
            scanf("%s", search_term);
            getchar(); // Consume newline

            handle_search_files(sock, search_term, jobj);
        }
        else if (strcmp(command, "EXIT") == 0)
        {
            printf("Exiting the program...\n");
            handle_exit(sock, jobj);

            break;
        }
        else
        {
            printf("Command not recognized!\n");
            printf("Use command \"HELP\" to show the usage!\n");
            json_object_put(jobj); // Free the JSON object
        }
    }

    close(sock);
    return 0;
}

void handle_exit(int sock, struct json_object *jobj)
{
    json_object_object_add(jobj, "messageType", json_object_new_string("EXIT"));
    json_object_object_add(jobj, "payload", json_object_new_object());

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);
}

void handle_login(int sock, const char *username, const char *password, int *is_logged_in, char *current_user, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "userName", json_object_new_string(username));
    json_object_object_add(jpayload, "password", json_object_new_string(password));
    json_object_object_add(jobj, "messageType", json_object_new_string("LOGIN"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response and update login status
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    if (json_object_get_int(response_code) == 200)
    {
        *is_logged_in = 1;
        strncpy(current_user, username, MAX_USERNAME - 1);
        printf("Login success!\n");
    }
    else
    {
        printf("Wrong Username or Password!\n");
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_register(int sock, const char *username, const char *password, int *is_logged_in, char *current_user, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "userName", json_object_new_string(username));
    json_object_object_add(jpayload, "password", json_object_new_string(password));
    json_object_object_add(jobj, "messageType", json_object_new_string("REGISTER"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    if (json_object_get_int(response_code) == 201)
    {
        *is_logged_in = 1;
        strncpy(current_user, username, MAX_USERNAME - 1);
        printf("Registration successful! Hello %s\n", username);
    }
    else
    {
        printf("User already exists!\n");
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_create_group(int sock, const char *group_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("CREATE_GROUP"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];
    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    if (response_code_int == 409)
    {
        printf("Group existed!\n");
    }
    else if (response_code_int == 501)
    {
        printf("Server error!\n");
    }
    else if (response_code_int == 201)
    {
        printf("Create success!\n");
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_list_all_groups(int sock, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jobj, "messageType", json_object_new_string("LIST_ALL_GROUPS"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(buffer);

    print_group_list(parsed_json);

    json_object_put(parsed_json); // Free the JSON object
}

void handle_join_group(int sock, const char *group_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("JOIN_GROUP"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Request successfully!\n");
        break;
    case 404:
        printf("Group does not exist!\n");
        break;
    case 409:
        printf("You are a member in group!\n");
        break;
    case 429:
        printf("Too many requests!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        printf("Unknown code! (%d)\n", response_code_int);
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_invite_to_group(int sock, const char *group_name, const char *invited_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jpayload, "invitedName", json_object_new_string(invited_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("INVITE_TO_GROUP"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Invite member successfully!\n");
        break;
    case 409:
        printf("Member is already in group!\n");
        break;
    case 403:
        printf("You are not a member in group!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        printf("Unknown code! (%d)\n", response_code_int);
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_leave_group(int sock, const char *group_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("LEAVE_GROUP"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
}

void handle_list_group_members(int sock, const char *group_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("LIST_GROUP_MEMBERS"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;
    struct json_object *list_of_members_array;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        json_object_object_get_ex(parsed_json, "payload", &payload);
        json_object_object_get_ex(payload, "listOfMembers", &list_of_members_array);
        print_table_member(list_of_members_array, group_name);
        break;
    default:
        printf("You are not a member in group!\n");
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void print_table_member(struct json_object *list_of_members_array, const char *group_name)
{
    printf("\n┌──────────────────┐\n");
    printf("│ %-16s │\n", "Members");
    printf("├──────────────────┤\n");

    int array_len = json_object_array_length(list_of_members_array);
    for (int i = 0; i < array_len; i++)
    {
        struct json_object *member = json_object_array_get_idx(list_of_members_array, i);
        printf("│ %-16s │\n", json_object_get_string(member));
    }

    printf("└──────────────────┘\n");
    printf("Group: %s\n\n", group_name);
}

void handle_list_invitations(int sock, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jobj, "messageType", json_object_new_string("LIST_INVITATION"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE] = {0};
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json = json_tokener_parse(buffer);
    if (!parsed_json)
    {
        printf("Failed to parse server response\n");
        return;
    }

    struct json_object *response_code = NULL;
    if (!json_object_object_get_ex(parsed_json, "responseCode", &response_code))
    {
        printf("Invalid server response format\n");
        json_object_put(parsed_json);
        return;
    }

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
    {
        struct json_object *payload = NULL;
        struct json_object *list_of_invitations = NULL;

        if (json_object_object_get_ex(parsed_json, "payload", &payload) &&
            json_object_object_get_ex(payload, "listOfInvitations", &list_of_invitations) &&
            json_object_get_type(list_of_invitations) == json_type_array)
        {
            print_invitation_list(list_of_invitations);
        }
        else
        {
            printf("No invitations found\n");
        }
        break;
    }
    default:
        printf("You do not have any invitation!\n");
        break;
    }

    json_object_put(parsed_json);
}

void print_invitation_list(struct json_object *list_of_invitations)
{
    printf("\n┌──────────────────┐\n");
    printf("│ %-16s │\n", "Invitations");
    printf("├──────────────────┤\n");

    int array_len = json_object_array_length(list_of_invitations);
    for (int i = 0; i < array_len; i++)
    {
        struct json_object *invitation = json_object_array_get_idx(list_of_invitations, i);
        printf("│ %-16s │\n", json_object_get_string(invitation));
    }

    printf("└──────────────────┘\n\n");
}

void handle_remove_member(int sock, const char *group_name, const char *member_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jpayload, "memberName", json_object_new_string(member_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("REMOVE_MEMBER"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Remove member successfully!\n");
        break;
    case 404:
        printf("Member does not exist!\n");
        break;
    case 403:
        printf("You are not admin of this group!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        printf("You cannot remove yourself!\n");
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_approval(int sock, const char *group_name, const char *requester, const char *decision, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jpayload, "requester", json_object_new_string(requester));

    if (strcmp(decision, "YES") == 0 || strcmp(decision, "Y") == 0)
    {
        json_object_object_add(jpayload, "decision", json_object_new_string("ACCEPT"));
    }
    else if (strcmp(decision, "NO") == 0 || strcmp(decision, "N") == 0)
    {
        json_object_object_add(jpayload, "decision", json_object_new_string("DENIAL"));
    }
    else
    {
        printf("WRONG DECISION!\n");
        json_object_put(jobj); // Free the JSON object
        return;
    }

    json_object_object_add(jobj, "messageType", json_object_new_string("APPROVAL"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Approval successfully!\n");
        break;
    case 400:
        printf("Bad request!\n");
        break;
    case 403:
        printf("You are not admin of group %s!\n", group_name);
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        printf("Unknown code! (%d)\n", response_code_int);
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_join_request_status(int sock, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jobj, "messageType", json_object_new_string("JOIN_REQUEST_STATUS"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;
    struct json_object *join_request_status_array;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        json_object_object_get_ex(parsed_json, "payload", &payload);
        json_object_object_get_ex(payload, "listOfAppliedGroups", &join_request_status_array);
        print_join_request_status(join_request_status_array);
        break;
    case 403:
        printf("You are not an admin of group!\n");
        break;
    case 201:
        printf("You do not have any request!\n");
        break;
    default:
        printf("You are not an admin in group!\n");
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void print_join_request_status(struct json_object *join_request_status_array)
{
    printf("Join Request Status:\n");
    int array_len = json_object_array_length(join_request_status_array);
    for (int i = 0; i < array_len; i++)
    {
        struct json_object *request_status = json_object_array_get_idx(join_request_status_array, i);
        printf("%s\n", json_object_get_string(request_status));
    }
}

void handle_join_request_list(int sock, const char *group_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("JOIN_REQUEST_LIST"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;
    struct json_object *join_request_list_array;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        json_object_object_get_ex(parsed_json, "payload", &payload);
        json_object_object_get_ex(payload, "joinRequestList", &join_request_list_array);
        print_join_request_list(join_request_list_array, group_name);
        break;
    case 403:
        printf("You are not an admin of group!\n");
        break;
    case 201:
        printf("Group does not have any request!\n");
        break;
    default:
        printf("You are not an admin in group!\n");
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void print_join_request_list(struct json_object *join_request_list_array, const char *group_name)
{
    printf("Join Request List for group %s:\n", group_name);
    int array_len = json_object_array_length(join_request_list_array);
    for (int i = 0; i < array_len; i++)
    {
        struct json_object *request = json_object_array_get_idx(join_request_list_array, i);
        printf("%s\n", json_object_get_string(request));
    }
}

void handle_folder_content(int sock, const char *group_name, const char *folder_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jpayload, "folderName", json_object_new_string(folder_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("FOLDER_CONTENT"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;
    struct json_object *folder_content_array;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        json_object_object_get_ex(parsed_json, "payload", &payload);
        json_object_object_get_ex(payload, "folderContents", &folder_content_array);
        print_table_files(folder_content_array, folder_name, group_name);
        break;
    case 404:
        printf("Folder does not exist!\n");
        break;
    default:
        printf("You are not a member in group!\n");
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void print_table_files(struct json_object *folder_content_array, const char *folder_name, const char *group_name)
{
    printf("Contents of folder %s in group %s:\n", folder_name, group_name);
    int array_len = json_object_array_length(folder_content_array);
    for (int i = 0; i < array_len; i++)
    {
        struct json_object *file = json_object_array_get_idx(folder_content_array, i);
        printf("%s\n", json_object_get_string(file));
    }
}

void handle_create_folder(int sock, const char *folder_path, const char *folder_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "folderPath", json_object_new_string(folder_path));
    json_object_object_add(jpayload, "folderName", json_object_new_string(folder_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("CREATE_FOLDER"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 201:
        printf("Create folder success!\n");
        break;
    case 409:
        printf("Folder existed!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_folder_rename(int sock, const char *folder_path, const char *new_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "folderPath", json_object_new_string(folder_path));
    json_object_object_add(jpayload, "newFolderName", json_object_new_string(new_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("FOLDER_RENAME"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Rename folder successfully!\n");
        break;
    case 404:
        printf("Folder does not exist!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_folder_copy(int sock, const char *from_folder, const char *to_folder, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "fromFolder", json_object_new_string(from_folder));
    json_object_object_add(jpayload, "toFolder", json_object_new_string(to_folder));
    json_object_object_add(jobj, "messageType", json_object_new_string("FOLDER_COPY"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Copy folder successfully!\n");
        break;
    case 404:
        printf("Folder does not exist!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_folder_move(int sock, const char *from_folder, const char *to_folder, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "fromFolder", json_object_new_string(from_folder));
    json_object_object_add(jpayload, "toFolder", json_object_new_string(to_folder));
    json_object_object_add(jobj, "messageType", json_object_new_string("FOLDER_MOVE"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Move folder successfully!\n");
        break;
    case 404:
        printf("Folder does not exist!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_folder_delete(int sock, const char *folder_path, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "folderPath", json_object_new_string(folder_path));
    json_object_object_add(jobj, "messageType", json_object_new_string("FOLDER_DELETE"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Delete folder successfully!\n");
        break;
    case 404:
        printf("Folder does not exist!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_search_folders(int sock, const char *folder_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "folderName", json_object_new_string(folder_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("FOLDER_SEARCH"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    // TODO: Implement handle response
    switch (response_code_int)
    {
    case 200:
        printf("Search folder successfully!\n");
        break;
    case 404:
        printf("Folder does not exist!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_download_folder(int sock, const char *folder_path, const char *folder_owner, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "folderPath", json_object_new_string(folder_path));
    json_object_object_add(jpayload, "folderOwner", json_object_new_string(folder_owner));
    json_object_object_add(jobj, "messageType", json_object_new_string("FOLDER_DOWNLOAD"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    // TODO: Implement handle response
    switch (response_code_int)
    {
    case 200:
        printf("Download folder successfully!\n");
        break;
    case 404:
        printf("Folder does not exist!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_upload_folder(int sock, const char *folder_path, const char *folder_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "folderName", json_object_new_string(folder_name));
    json_object_object_add(jpayload, "folderPath", json_object_new_string(folder_path));
    json_object_object_add(jobj, "messageType", json_object_new_string("FOLDER_UPLOAD"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Upload folder successfully!\n");
        break;
    case 404:
        printf("Folder does not exist!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_upload_file(int sock, const char *folder_path, const char *file_path, struct json_object *jobj)
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

    json_object_object_add(jpayload, "folderPath", json_object_new_string(folder_path));
    json_object_object_add(jpayload, "fileName", json_object_new_string(get_filename(file_path)));
    json_object_object_add(jpayload, "fileSize", json_object_new_int64(file_size));
    json_object_object_add(jobj, "messageType", json_object_new_string("UPLOAD_FILE"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
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
        printf("\nUploaded!\n");
        break;
    case 409:
        printf("File exists!\n");
        break;
    default:
        printf("Unknown error!\n");
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_download_file(int sock, const char *file_path, const char *file_owner, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "filePath", json_object_new_string(file_path));
    json_object_object_add(jpayload, "fileOwner", json_object_new_string(file_owner));
    json_object_object_add(jobj, "messageType", json_object_new_string("DOWNLOAD_FILE"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        json_object_object_get_ex(parsed_json, "payload", &payload);
        struct json_object *file_size_obj;
        json_object_object_get_ex(payload, "fileSize", &file_size_obj);
        long file_size = json_object_get_int64(file_size_obj);

        if (file_size != 0)
        {
            char path[MAX_PATH_LENGTH];
            printf("Input destination path: ");
            scanf("%s", path);
            getchar(); // Consume newline

            struct stat st = {0};
            if (stat(path, &st) == -1)
            {
                mkdir(path, 0777);
            }

            char des_path[MAX_PATH_LENGTH];
            char *file_name = get_filename(file_path);
            snprintf(des_path, sizeof(des_path), "%s/%s", path, file_name);

            FILE *f = fopen(des_path, "wb");
            if (!f)
            {
                printf("Failed to open destination file!\n");
                break;
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
        break;
    default:
        printf("Unknown error!\n");
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_file_rename(int sock, const char *file_path, const char *new_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "filePath", json_object_new_string(file_path));
    json_object_object_add(jpayload, "newFileName", json_object_new_string(new_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("FILE_RENAME"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Rename file successfully!\n");
        break;
    case 404:
        printf("Folder or File does not exist!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_file_copy(int sock, const char *file_path, const char *to_folder, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "filePath", json_object_new_string(file_path));
    json_object_object_add(jpayload, "toFolder", json_object_new_string(to_folder));
    json_object_object_add(jobj, "messageType", json_object_new_string("FILE_COPY"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Copy file successfully!\n");
        break;
    case 404:
        printf("Folder or File does not exist!\n");
        break;
    case 403:
        printf("You are not a member in group!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_file_move(int sock, const char *file_path, const char *to_folder, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "filePath", json_object_new_string(file_path));
    json_object_object_add(jpayload, "toFolder", json_object_new_string(to_folder));
    json_object_object_add(jobj, "messageType", json_object_new_string("FILE_MOVE"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Move file successfully!\n");
        break;
    case 404:
        printf("Folder or File does not exist!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_file_delete(int sock, const char *file_path, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "filePath", json_object_new_string(file_path));
    json_object_object_add(jobj, "messageType", json_object_new_string("FILE_DELETE"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object

    char buffer[BUFFER_SIZE];

    // Check response
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Delete file successfully!\n");
        break;
    case 404:
        printf("Folder or File does not exist!\n");
        break;
    case 501:
        printf("Server error!\n");
        break;
    default:
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void handle_search_files(int sock, const char *file_name, struct json_object *jobj)
{
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "fileName", json_object_new_string(file_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("FILE_SEARCH"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    char buffer[BUFFER_SIZE];
    recv(sock, buffer, BUFFER_SIZE, 0);

    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;
    struct json_object *file_list_array;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);

    int response_code_int = json_object_get_int(response_code);
    switch (response_code_int)
    {
    case 200:
        printf("Search file successfully!\n");
        break;
    case 404:
        printf("File does not exist!\n");
        break;
    default:
        printf("You are not a member in group!\n");
        break;
    }

    json_object_put(parsed_json); // Free the JSON object
}

void print_usage(void)
{
    printf("Commands:\n");
    printf("LOGIN - Log in to the system\n");
    printf("REGISTER - Register a new user\n");
    printf("LOGOUT - Log out of the system\n");
    printf("FOLDER_CONTENT - List all files in a folder\n");
    printf("CREATE_FOLDER - Create a new folder\n");
    printf("FOLDER_RENAME - Rename a folder\n");
    printf("FOLDER_COPY - Copy a folder to another group\n");
    printf("FOLDER_MOVE - Move a folder to another group\n");
    printf("FOLDER_DELETE - Delete a folder\n");
    printf("SEARCH_FOLDERS - Search for a folder\n");
    printf("DOWNLOAD_FOLDER - Download a folder\n");
    printf("UPLOAD_FOLDER - Upload a folder\n");
    printf("UPLOAD_FILE - Upload a file to a folder\n");
    printf("DOWNLOAD_FILE - Download a file from a folder\n");
    printf("FILE_RENAME - Rename a file\n");
    printf("FILE_COPY - Copy a file to another group\n");
    printf("FILE_MOVE - Move a file to another group\n");
    printf("FILE_DELETE - Delete a file\n");
    printf("SEARCH_FILES - Search for a file\n");
    printf("EXIT - Exit the program\n");
    printf("HELP - Show usage\n");
}

const char *get_filename(const char *path)
{
    const char *filename = strrchr(path, '/');
    if (filename == NULL)
    {
        return path; // No '/' found, return the original path
    }
    return filename + 1; // Skip the '/' character
}

void print_group_list(struct json_object *parsed_json)
{
    struct json_object *payload, *list_groups_array;

    // Navigate through the JSON structure
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "listGroups", &list_groups_array);

    printf("\n┌──────────────┬──────────────┬─────────────────────┐\n");
    printf("│ Group Name   │ Created By   │ Created At          │\n");
    printf("├──────────────┼──────────────┼─────────────────────┤\n");

    int array_len = json_object_array_length(list_groups_array);
    for (int i = 0; i < array_len; i++)
    {
        struct json_object *group = json_object_array_get_idx(list_groups_array, i);
        struct json_object *group_name, *created_by, *created_at;

        json_object_object_get_ex(group, "groupName", &group_name);
        json_object_object_get_ex(group, "createdBy", &created_by);
        json_object_object_get_ex(group, "createdAt", &created_at);

        printf("│ %-12s │ %-12s │ %-19s │\n",
               json_object_get_string(group_name),
               json_object_get_string(created_by),
               json_object_get_string(created_at));
    }
    printf("└──────────────┴──────────────┴─────────────────────┘\n\n");
}