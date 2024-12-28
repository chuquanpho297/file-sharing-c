#ifndef CLIENT_H
#define CLIENT_H

// Function prototypes
void print_usage(void);
char *clean_file_path(const char *path);
void handle_login(int sock, const char *username, const char *password,
                  int *is_logged_in, char *current_user, struct json_object *j);
void handle_register(int sock, const char *username, const char *password,
                     int *is_logged_in, char *current_user,
                     struct json_object *j);
// void handle_folder_content(int sock, const char *group_name, const char
// *folder_name, struct json_object *j); void print_table_files(struct
// json_object *folder_content_array, const char *folder_name, const char
// *group_name);
void handle_folder_create_client(int sock, const char *folder_path,
                                 const char *folder_name,
                                 struct json_object *j);
void handle_folder_rename_client(int sock, const char *folder_path,
                                 const char *new_name, struct json_object *j);
void handle_folder_copy_client(int sock, const char *from_folder,
                               const char *to_folder, struct json_object *j);
void handle_folder_move_client(int sock, const char *from_folder,
                               const char *to_folder, struct json_object *j);
void handle_folder_delete_client(int sock, const char *folder_path,
                                 struct json_object *j);
void handle_folder_search_client(int sock, const char *search_term,
                                 struct json_object *j);
void handle_folder_download_client(int sock, const char *folder_path,
                                   const char *folder_owner,
                                   const char *des_folder_path,
                                   struct json_object *j);
void handle_folder_upload_client(int sock, const char *folder_path,
                                 const char *folder_name,
                                 struct json_object *j);
void handle_file_upload_client(int sock, const char *folder_path,
                               const char *file_path, struct json_object *j);
void handle_file_download_client(int sock, const char *file_path,
                                 const char *file_owner, struct json_object *j);
void handle_file_rename_client(int sock, const char *file_path,
                               const char *new_name, struct json_object *j);
void handle_file_copy_client(int sock, const char *file_path,
                             const char *to_folder, struct json_object *j);
void handle_file_move_client(int sock, const char *file_path,
                             const char *to_folder, struct json_object *j);
void handle_file_delete_client(int sock, const char *file_path,
                               struct json_object *j);
void handle_file_search_client(int sock, const char *file_name,
                               struct json_object *jobj);
void handle_set_folder_access(int sock, const char *folder_path,
                              const char *access, struct json_object *jobj);
void handle_logout(int sock, int *is_logged_in, char *current_user);
void handle_file_search_client(int sock, const char *search_term,
                               struct json_object *j);
void send_folder(int sock, const char *upload_folder_path,
                 const char *des_folder_path);
void handle_set_file_access(int sock, const char *file_path, const char *access,
                            struct json_object *jobj);
const char *get_filename(const char *path);

#endif  // CLIENT_H