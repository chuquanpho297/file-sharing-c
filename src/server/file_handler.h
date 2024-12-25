#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <json-c/json.h>
#include "../utils/structs.h"

// File operations handlers
void handle_file_create(client_t* client, const char* buffer);
void handle_file_download(client_t* client, const char* buffer);
void handle_file_copy(client_t* client, const char* buffer);
void handle_file_move(client_t* client, const char* buffer);
void handle_file_rename(client_t* client, const char* buffer);
void handle_file_delete(client_t* client, const char* buffer);

// Access control handlers
void handle_file_set_access(client_t* client, const char* buffer);
void handle_file_get_access(client_t* client, const char* buffer);

// Search handlers
void handle_file_search(client_t* client, const char* buffer);

#endif // FILE_HANDLER_H