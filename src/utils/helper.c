#include "helper.h"

#include <arpa/inet.h>
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

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