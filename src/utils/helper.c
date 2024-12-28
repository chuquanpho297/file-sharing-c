#include "helper.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>  // for malloc
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>   // for stat()
#include <sys/types.h>  // for struct stat
#include <unistd.h>     // for unlink()

#include "structs.h"

void clear_line()
{
    printf("\r\033[K");  // Clear current line
    fflush(stdout);
}

void send_response(int socket, int code, const char *message)
{
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();

    json_object_object_add(jobj, "responseCode", json_object_new_int(code));
    json_object_object_add(jpayload, "message",
                           json_object_new_string(message));
    json_object_object_add(jobj, "payload", jpayload);

    const char *response_str = json_object_to_json_string(jobj);
    send(socket, response_str, strlen(response_str), 0);

    json_object_put(jobj);
}

void handle_print_payload_response(char *buffer,
                                   PrintPayloadResponseFunc print_func)
{
    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);
    json_object_object_get_ex(parsed_json, "payload", &payload);
    int response_code_int = json_object_get_int(response_code);
    print_func(response_code_int, payload);

    json_object_put(parsed_json);  // Free the JSON object
}

void print_message_oneline(int response_code, struct json_object *payload)
{
    struct json_object *message;
    json_object_object_get_ex(payload, "message", &message);
    const char *message_str = json_object_get_string(message);
    printf("<%d>: %s\n", response_code, message_str);
}

char *handle_response_chunk(int sock, int max_size)
{
    int total_received = 0;
    int bytes_received;
    int buffer_size = max_size;
    char *response = (char *)malloc(buffer_size);
    char buffer[max_size];

    if (response == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    while ((bytes_received = recv(sock, buffer, max_size, 0)) > 0)
    {
        if (total_received + bytes_received >= buffer_size)
        {
            buffer_size *= 2;
            response = (char *)realloc(response, buffer_size);
            if (response == NULL)
            {
                fprintf(stderr, "Memory reallocation failed\n");
                return NULL;
            }
        }
        memcpy(response + total_received, buffer, bytes_received);
        total_received += bytes_received;
        if (bytes_received < max_size)
        {
            break;
        }
    }

    response[total_received] = '\0';
    return response;
}

void print_file_table(struct json_object *files)
{
    printf("%-36s %-16s  %-6s  %-6s  %-9s  %-19s  %s \n", "File ID",
           "File Name", "Size", "Access", "Created By", "Created At",
           "File Path");

    int array_size = json_object_array_length(files);
    for (int i = 0; i < array_size; i++)
    {
        struct json_object *file = json_object_array_get_idx(files, i);

        struct json_object *file_id = json_object_object_get(file, "fileId");

        struct json_object *file_name =
            json_object_object_get(file, "fileName");
        struct json_object *file_size =
            json_object_object_get(file, "fileSize");
        struct json_object *access = json_object_object_get(file, "access");
        struct json_object *created_by =
            json_object_object_get(file, "createdBy");
        struct json_object *created_at =
            json_object_object_get(file, "createdAt");
        struct json_object *file_path =
            json_object_object_get(file, "filePath");
        printf(
            "%-36s %-16s  %-6s  %-6s  %-9s  %-19s  %s\n",
            json_object_get_string(file_id), json_object_get_string(file_name),
            json_object_get_string(file_size), json_object_get_string(access),
            json_object_get_string(created_by),
            json_object_get_string(created_at),
            json_object_get_string(file_path));
    }
    // Print table rows
}

void fgets_not_newline(char *buffer, int size)
{
    if (fgets(buffer, size, stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove trailing newline
    }
}

int remove_directory(const char *path)
{
    DIR *dir = opendir(path);
    size_t path_len = strlen(path);
    int result = -1;

    if (dir)
    {
        struct dirent *entry;
        result = 0;

        while (!result && (entry = readdir(dir)))
        {
            char *buf;
            size_t len;

            // Skip the names "." and ".." as we don't want to recurse on them
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                continue;

            len = path_len + strlen(entry->d_name) + 2;
            buf = malloc(len);

            if (buf)
            {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, entry->d_name);

                if (!stat(buf, &statbuf))
                {
                    if (S_ISDIR(statbuf.st_mode))
                    {
                        // Recursive call for directories
                        result = remove_directory(buf);
                    }
                    else
                    {
                        // Remove regular files
                        result = unlink(buf);
                    }
                }
                free(buf);
            }
        }
        closedir(dir);
    }

    if (!result)
    {
        // Remove the empty directory
        result = rmdir(path);
    }

    return result;
}