#include "../../utils/structs.h"
#include <sys/stat.h>
#include <stdio.h>
#ifndef USER_HANDLER_H
#define USER_HANDLER_H

// Function prototypes
void handle_login(client_t *client, const char *buffer);
void handle_register(client_t *client, const char *buffer);

#endif  // USER_HANDLER_H