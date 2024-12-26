
#ifndef HELPER_H
#define HELPER_H

#include <json-c/json.h>

typedef void (*PrintPayloadResponseFunc)(int, struct json_object *);

void clear_line();
void send_response(int socket, int code, const char *message);

void handle_print_payload_response(char *buffer,
                                   PrintPayloadResponseFunc print_func);
void print_message_oneline(int response_code, struct json_object *payload);
char *handle_response_chunk(int sock, int max_size);

#define CHECK_ERROR(cond, client, msg)           \
    if (cond)                                    \
    {                                            \
        send_response(client->socket, 500, msg); \
        return;                                  \
    }

void print_file_table(struct json_object *files);

#endif  // HELPER_H