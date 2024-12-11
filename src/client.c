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
#define MAX_GROUP_NAME 32

// Function prototypes
void print_usage(void);
void handle_login(int sock, const char *username, const char *password);
void handle_register(int sock, const char *username, const char *password);
void handle_create_group(int sock, const char *group_name);
void handle_list_all_groups(int sock);
void handle_join_group(int sock, const char *group_name);
void handle_invite_to_group(int sock, const char *group_name, const char *invited_name);
void handle_leave_group(int sock, const char *group_name);
void handle_list_group_members(int sock, const char *group_name);
void print_table_member(struct json_object *list_of_members_array, const char *group_name);
void handle_list_invitations(int sock);
void print_invitation_list(struct json_object *list_of_invitations);
void handle_remove_member(int sock, const char *group_name, const char *member_name);
void handle_approval(int sock, const char *group_name, const char *requester, const char *decision);
void handle_join_request_status(int sock);
void print_join_request_status(struct json_object *join_request_status_array);
void handle_join_request_list(int sock, const char *group_name);
void print_join_request_list(struct json_object *join_request_list_array, const char *group_name);
void handle_folder_content(int sock, const char *group_name, const char *folder_name);
void print_table_files(struct json_object *folder_content_array, const char *folder_name, const char *group_name);
void handle_create_folder(int sock, const char *group_name, const char *folder_name);

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
        else if (strcmp(command, "CREATE_GROUP") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to create a group. Please log in.\n");
                continue;
            }

            char group_name[MAX_GROUP_NAME];
            printf("Enter group name: ");
            scanf("%s", group_name);
            getchar(); // Consume newline

            handle_create_group(sock, group_name);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Response from server:\n%s\n", buffer);

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
        else if (strcmp(command, "LIST_ALL_GROUPS") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to see list groups. Please log in!\n");
                continue;
            }

            handle_list_all_groups(sock);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("%s\n", buffer);

            struct json_object *parsed_json;
            parsed_json = json_tokener_parse(buffer);

            printf("Group List:\n%s\n", json_object_to_json_string(parsed_json));

            json_object_put(parsed_json); // Free the JSON object
        }
        else if (strcmp(command, "JOIN_GROUP") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to request to join a group. Please log in!\n");
                continue;
            }

            char group_name[MAX_GROUP_NAME];
            printf("Enter group name: ");
            scanf("%s", group_name);
            getchar(); // Consume newline

            handle_join_group(sock, group_name);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("%s\n", buffer);

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
        else if (strcmp(command, "INVITE_TO_GROUP") == 0)
        {
            if (!is_logged_in)
            {
                printf("Please login to use this function!\n");
                continue;
            }

            char group_name[MAX_GROUP_NAME];
            char invited_name[MAX_USERNAME];
            printf("Enter group name: ");
            scanf("%s", group_name);
            getchar(); // Consume newline
            printf("Enter username who you want to invite: ");
            scanf("%s", invited_name);
            getchar(); // Consume newline

            handle_invite_to_group(sock, group_name, invited_name);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Response from server:\n%s\n", buffer);

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
        else if (strcmp(command, "LIST_GROUP_MEMBERS") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to see members in this group. Please log in.\n");
                continue;
            }

            char group_name[MAX_GROUP_NAME];
            printf("Enter group name: ");
            scanf("%s", group_name);
            getchar(); // Consume newline

            handle_list_group_members(sock, group_name);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Response from server:\n%s\n", buffer);

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
        else if (strcmp(command, "LIST_INVITATION") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to see list groups. Please log in!\n");
                continue;
            }

            handle_list_invitations(sock);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Response from server:\n%s\n", buffer);

            struct json_object *parsed_json;
            struct json_object *response_code;
            struct json_object *payload;
            struct json_object *list_of_invitations;

            parsed_json = json_tokener_parse(buffer);
            json_object_object_get_ex(parsed_json, "responseCode", &response_code);

            int response_code_int = json_object_get_int(response_code);
            switch (response_code_int)
            {
            case 200:
                json_object_object_get_ex(parsed_json, "payload", &payload);
                json_object_object_get_ex(payload, "listOfInvitation", &list_of_invitations);
                print_invitation_list(list_of_invitations);
                break;
            default:
                printf("You do not have any invitation!\n");
                break;
            }

            json_object_put(parsed_json); // Free the JSON object
        }
        else if (strcmp(command, "REMOVE_MEMBER") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to remove a member. Please log in.\n");
                continue;
            }

            char group_name[MAX_GROUP_NAME];
            char member_name[MAX_USERNAME];
            printf("Enter group name: ");
            scanf("%s", group_name);
            getchar(); // Consume newline
            printf("Enter member name: ");
            scanf("%s", member_name);
            getchar(); // Consume newline

            handle_remove_member(sock, group_name, member_name);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Response from server:\n%s\n", buffer);

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
        else if (strcmp(command, "APPROVAL") == 0)
        {
            if (!is_logged_in)
            {
                printf("Please login to use this function!\n");
                continue;
            }

            char group_name[MAX_GROUP_NAME];
            char requester[MAX_USERNAME];
            char decision[4]; // To accommodate "YES" or "NO"
            printf("Enter group name: ");
            scanf("%s", group_name);
            getchar(); // Consume newline
            printf("Enter requester: ");
            scanf("%s", requester);
            getchar(); // Consume newline
            printf("Decision[YES/NO]: ");
            scanf("%s", decision);
            getchar(); // Consume newline

            handle_approval(sock, group_name, requester, decision);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Response from server:\n%s\n", buffer);

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
        else if (strcmp(command, "JOIN_REQUEST_STATUS") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to see list groups. Please log in!\n");
                continue;
            }

            handle_join_request_status(sock);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Response from server:\n%s\n", buffer);

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
        else if (strcmp(command, "JOIN_REQUEST_LIST") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to see members in this group. Please log in.\n");
                continue;
            }

            char group_name[MAX_GROUP_NAME];
            printf("Enter group name: ");
            scanf("%s", group_name);
            getchar(); // Consume newline

            handle_join_request_list(sock, group_name);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Response from server:\n%s\n", buffer);

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
        else if (strcmp(command, "FOLDER_CONTENT") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to see folder content in this group. Please log in.\n");
                continue;
            }

            char group_name[MAX_GROUP_NAME];
            char folder_name[MAX_GROUP_NAME];
            printf("Enter group name: ");
            scanf("%s", group_name);
            getchar(); // Consume newline
            printf("Enter folder name: ");
            scanf("%s", folder_name);
            getchar(); // Consume newline

            handle_folder_content(sock, group_name, folder_name);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Response from server:\n%s\n", buffer);

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
        else if (strcmp(command, "CREATE_FOLDER") == 0)
        {
            if (!is_logged_in)
            {
                printf("You do not have permission to create a folder. Please log in!\n");
                continue;
            }

            char group_name[MAX_GROUP_NAME];
            char folder_name[MAX_GROUP_NAME];
            printf("Enter group name: ");
            scanf("%s", group_name);
            getchar(); // Consume newline
            printf("Enter folder name: ");
            scanf("%s", folder_name);
            getchar(); // Consume newline

            handle_create_folder(sock, group_name, folder_name);

            // Check response
            recv(sock, buffer, BUFFER_SIZE, 0);
            printf("Response from server:\n%s\n", buffer);

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

void handle_create_group(int sock, const char *group_name)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("CREATE_GROUP"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
}

void handle_list_all_groups(int sock)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jobj, "messageType", json_object_new_string("LIST_ALL_GROUPS"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
}

void handle_join_group(int sock, const char *group_name)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("JOIN_GROUP"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
}

void handle_invite_to_group(int sock, const char *group_name, const char *invited_name)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jpayload, "invitedName", json_object_new_string(invited_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("INVITE_TO_GROUP"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
}

void handle_leave_group(int sock, const char *group_name)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("LEAVE_GROUP"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
}

void handle_list_group_members(int sock, const char *group_name)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("LIST_GROUP_MEMBERS"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
}

void print_table_member(struct json_object *list_of_members_array, const char *group_name)
{
    printf("Members of group %s:\n", group_name);
    int array_len = json_object_array_length(list_of_members_array);
    for (int i = 0; i < array_len; i++)
    {
        struct json_object *member = json_object_array_get_idx(list_of_members_array, i);
        printf("%s\n", json_object_get_string(member));
    }
}

void handle_list_invitations(int sock)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jobj, "messageType", json_object_new_string("LIST_INVITATION"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
}

void print_invitation_list(struct json_object *list_of_invitations)
{
    printf("List of Invitations:\n");
    int array_len = json_object_array_length(list_of_invitations);
    for (int i = 0; i < array_len; i++)
    {
        struct json_object *invitation = json_object_array_get_idx(list_of_invitations, i);
        printf("%s\n", json_object_get_string(invitation));
    }
}

void handle_remove_member(int sock, const char *group_name, const char *member_name)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jpayload, "memberName", json_object_new_string(member_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("REMOVE_MEMBER"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
}

void handle_approval(int sock, const char *group_name, const char *requester, const char *decision)
{
    struct json_object *jobj = json_object_new_object();
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
}

void handle_join_request_status(int sock)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jobj, "messageType", json_object_new_string("JOIN_REQUEST_STATUS"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
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

void handle_join_request_list(int sock, const char *group_name)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("JOIN_REQUEST_LIST"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
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

void handle_folder_content(int sock, const char *group_name, const char *folder_name)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jpayload, "folderName", json_object_new_string(folder_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("FOLDER_CONTENT"));
    json_object_object_add(jobj, "payload", jpayload);

    const char *request = json_object_to_json_string(jobj);
    send(sock, request, strlen(request), 0);

    json_object_put(jobj); // Free the JSON object
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

void handle_create_folder(int sock, const char *group_name, const char *folder_name)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jpayload, "groupName", json_object_new_string(group_name));
    json_object_object_add(jpayload, "folderName", json_object_new_string(folder_name));
    json_object_object_add(jobj, "messageType", json_object_new_string("CREATE_FOLDER"));
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
    printf("CREATE_GROUP - Create a new group\n");
    printf("LIST_ALL_GROUPS - List all groups\n");
    printf("JOIN_GROUP - Request to join a group\n");
    printf("INVITE_TO_GROUP - Invite a user to a group\n");
    printf("LEAVE_GROUP - Leave a group\n");
    printf("LIST_GROUP_MEMBERS - List all members in a group\n");
    printf("LIST_INVITATION - List all invitations\n");
    printf("REMOVE_MEMBER - Remove a member from a group\n");
    printf("APPROVAL - Approve or deny a join request\n");
    printf("JOIN_REQUEST_STATUS - List all join request status\n");
    printf("JOIN_REQUEST_LIST - List all join requests for a group\n");
    printf("FOLDER_CONTENT - List all files in a folder\n");
    printf("CREATE_FOLDER - Create a new folder\n");
    printf("HELP - Show usage\n");
}