#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <stdbool.h>

typedef struct {
    int socket;
    char username[32];
    bool is_logged_in;
} client_t;

void send_response(int socket, int code, const char* message);

#endif 