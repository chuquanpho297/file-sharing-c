#ifndef HELPER_H
#define HELPER_H

void clear_line();
void send_response(int socket, int code, const char *message);

#define CHECK_ERROR(cond, client, msg)           \
    if (cond)                                    \
    {                                            \
        send_response(client->socket, 500, msg); \
        json_object_put(parsed_json);            \
        free(path_copy);                         \
        return;                                  \
    }

#endif  // HELPER_H