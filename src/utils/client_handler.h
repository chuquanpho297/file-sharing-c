#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <json-c/json.h>

// Auth handlers
void handle_login_client(int sock, const char* username, const char* password,
                         int* is_logged_in, char* current_user,
                         struct json_object* j);
void handle_register_client(int sock, const char* username,
                            const char* password, int* is_logged_in,
                            char* current_user, struct json_object* j);
void handle_logout_client(int sock, int* is_logged_in, char* current_user);

// Folder handlers
void handle_folder_create_client(int sock, const char* folder_path,
                                 const char* folder_name,
                                 struct json_object* j);
void handle_folder_rename_client(int sock, const char* folder_path,
                                 const char* new_name, struct json_object* j);
void handle_folder_copy_client(int sock, const char* from_folder,
                               const char* to_folder, struct json_object* j);
void handle_folder_move_client(int sock, const char* from_folder,
                               const char* to_folder, struct json_object* j);
void handle_folder_delete_client(int sock, const char* folder_path,
                                 struct json_object* j);
void handle_folder_search_client(int sock, const char* folder_name,
                                 struct json_object* j);
void handle_folder_download_client(int sock, const char* folder_path,
                                   const char* folder_owner,
                                   struct json_object* j);
void handle_folder_upload_client(int sock, const char* des_folder_path,
                                 const char* folder_path,
                                 struct json_object* j);

// File handlers
void handle_file_upload_client(int sock, const char* folder_path,
                               const char* file_path, struct json_object* j);
void handle_file_download_client(int sock, const char* file_path,
                                 const char* file_owner, struct json_object* j);
void handle_file_rename_client(int sock, const char* file_path,
                               const char* new_name, struct json_object* j);
void handle_file_copy_client(int sock, const char* file_path,
                             const char* to_folder, struct json_object* j);
void handle_file_move_client(int sock, const char* file_path,
                             const char* to_folder, struct json_object* j);
void handle_file_delete_client(int sock, const char* file_path,
                               struct json_object* j);
void handle_file_search_client(int sock, const char* file_name,
                               struct json_object* j);

// Helper functions
void send_folder(int sock, const char* upload_folder_path,
                 const char* des_folder_path);

#endif  // CLIENT_HANDLER_H