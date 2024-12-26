#include <json-c/json.h>

typedef void (*PrintPayloadResponseFunc)(int, struct json_object *);

void handle_print_payload_response(char *buffer,
                                   PrintPayloadResponseFunc print_func);

void print_message_oneline(int response_code, struct json_object *payload);

char *handle_response_chunk(int sock, int max_size);

const char *get_filename(const char *path);

const char *get_folder_name(const char *path);
