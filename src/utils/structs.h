#ifndef STRUCTS_H
#define STRUCTS_H

#define MAX_USERNAME 32
#define MAX_PASSWORD 32
#define MAX_CLIENTS 100
#define BUFFER_SIZE 4096
#define PORT 5555

typedef struct
{
    int socket;
    char username[MAX_USERNAME];
    int is_logged_in;
} client_t;

#endif