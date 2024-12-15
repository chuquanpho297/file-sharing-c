#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <json-c/json.h>
#include "controllers/file_controller.h"
#include "structs.h"

// File operation handlers
void handle_upload_file(client_t *client, const char *buffer);
void handle_download_file(client_t *client, const char *buffer);
void handle_file_rename(client_t *client, const char *buffer);
void handle_file_copy(client_t *client, const char *buffer);
void handle_file_move(client_t *client, const char *buffer);
void handle_file_delete(client_t *client, const char *buffer);

#endif