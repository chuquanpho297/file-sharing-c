#ifndef FOLDER_HANDLER_H
#define FOLDER_HANDLER_H

#include <json-c/json.h>

#include "../../utils/structs.h"

// Folder operations handlers
void handle_root_folder_create(const char *username);
void handle_folder_create(client_t *client, const char *buffer);
void handle_folder_upload(client_t *client, const char *buffer);
void handle_folder_download(client_t *client, const char *buffer);
void handle_folder_rename(client_t *client, const char *buffer);
void handle_folder_copy(client_t *client, const char *buffer);
void handle_folder_move(client_t *client, const char *buffer);
void handle_folder_delete(client_t *client, const char *buffer);
void handle_folder_content(client_t *client, const char *buffer);

// Access control handlers
void handle_folder_set_access(client_t *client, const char *buffer);
void handle_folder_get_access(client_t *client, const char *buffer);

// Search handlers
void handle_folder_search(client_t *client, const char *buffer);
void handle_root_folder_get(client_t *client, const char *buffer);

#endif  // FOLDER_HANDLER_H