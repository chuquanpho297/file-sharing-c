#ifndef FOLDER_HANDLER_H
#define FOLDER_HANDLER_H

#include <json-c/json.h>
#include "../utils/structs.h"

// Folder operations handlers
void handle_create_root_folder(client_t* client, const char* buffer);
void handle_create_folder(client_t* client, const char* buffer);
void handle_folder_rename(client_t* client, const char* buffer);
void handle_folder_copy(client_t* client, const char* buffer);
void handle_folder_move(client_t* client, const char* buffer);
void handle_folder_delete(client_t* client, const char* buffer);
void handle_folder_content(client_t* client, const char* buffer);

// Access control handlers
void handle_folder_set_access(client_t* client, const char* buffer);
void handle_folder_get_access(client_t* client, const char* buffer);

// Search handlers
void handle_search_folders(client_t* client, const char* buffer);
void handle_get_root_folder(client_t* client, const char* buffer);

#endif // FOLDER_HANDLER_H 