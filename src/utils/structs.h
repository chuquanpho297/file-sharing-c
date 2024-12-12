#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>

typedef struct
{
    int socket;
    char username[32];
    bool is_logged_in;
} client_t;

#endif