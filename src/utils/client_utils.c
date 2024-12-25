#include <stdio.h>
#include <json-c/json.h>
#include "./client_utils.h"

void handle_print_payload_response(char *buffer, PrintPayloadResponseFunc print_func)
{
    struct json_object *parsed_json;
    struct json_object *response_code;
    struct json_object *payload;

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "responseCode", &response_code);
    json_object_object_get_ex(parsed_json, "payload", &payload);
    int response_code_int = json_object_get_int(response_code);
    print_func(response_code, payload);

    json_object_put(parsed_json); // Free the JSON object
}

void print_message_oneline(int response_code, struct json_object *payload)
{
    struct json_object *message;
    json_object_object_get_ex(payload, "message", &message);
    const char *message_str = json_object_get_string(message);
    printf("<%d>\n%s\n", response_code, message_str);
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

const char *get_filename(const char *path)
{
    const char *filename = strrchr(path, '/');
    if (filename == NULL)
    {
        return path; // No '/' found, return the original path
    }
    return filename + 1; // Skip the '/' character
}
