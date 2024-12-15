#ifndef GROUP_HANDLER_H
#define GROUP_HANDLER_H

#include <json-c/json.h>
#include <sys/socket.h>
#include "../utils/structs.h"

void send_response(int socket, int code, const char* message);

void handle_create_group(client_t* client, const char* buffer);
void handle_list_all_groups(client_t* client, const char* buffer);
void handle_join_group(client_t* client, const char* buffer);
void handle_invite_to_group(client_t* client, const char* buffer);
void handle_leave_group(client_t* client, const char* buffer);
void handle_list_group_members(client_t* client, const char* buffer);
void handle_list_invitations(client_t* client, const char* buffer);
void handle_remove_member(client_t* client, const char* buffer);
void handle_approval(client_t* client, const char* buffer);
void handle_join_request_status(client_t* client, const char* buffer);
void handle_join_request_list(client_t* client, const char* buffer);

void handle_folder_content(client_t* client, const char* buffer);
void handle_create_folder(client_t* client, const char* buffer);
void handle_folder_rename(client_t* client, const char* buffer);
void handle_folder_copy(client_t* client, const char* buffer);
void handle_folder_move(client_t* client, const char* buffer);
void handle_folder_delete(client_t* client, const char* buffer);

#endif 