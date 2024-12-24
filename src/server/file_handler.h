#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <json-c/json.h>
#include "../utils/structs.h"

// File operations handlers
void handle_create_file(client_t* client, const char* buffer);
void handle_copy_file(client_t* client, const char* buffer);
void handle_move_file(client_t* client, const char* buffer);
void handle_rename_file(client_t* client, const char* buffer);
void handle_delete_file(client_t* client, const char* buffer);

// Access control handlers
void handle_file_set_access(client_t* client, const char* buffer);
void handle_file_get_access(client_t* client, const char* buffer);

// Search handlers
void handle_search_files(client_t* client, const char* buffer);

#endif // FILE_HANDLER_H