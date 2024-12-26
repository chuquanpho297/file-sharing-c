#include "helper.h"

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
