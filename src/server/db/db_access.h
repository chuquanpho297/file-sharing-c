#ifndef DB_ACCESS_H
#define DB_ACCESS_H

#include <mysql/mysql.h>
#include <stdbool.h>

#include "../../utils/structs.h"

// Database connection
MYSQL *db_connect(void);
void db_disconnect(MYSQL *conn);

// User operations
bool db_create_user(const char *user_name, const char *password);
bool db_login(const char *user_name, const char *password);

// Folder operations
bool db_create_root_folder(const char *user_name);
bool db_create_folder(const char *folder_name, const char *parent_folder_id,
                      const char *user_name);
char *db_get_root_folder_id(const char *user_name);
char *db_get_folder_id(const char *folder_name, const char *user_name,
                       const char *parent_folder_id);
char *db_get_parent_folder_id(const char *folder_name, const char *user_name);
bool db_copy_all_content_folder(const char *from_folder_id,
                                const char *to_folder_id);
// bool db_move_all_content_folder(const char *from_folder_id,
//                                 const char *to_folder_id);
bool db_copy_folder(const char *from_folder_id, const char *to_folder_id);

bool db_move_folder(const char *folder_id, const char *parent_folder_id,
                    const char *user_name);
bool db_delete_folder(const char *folder_id);
bool db_rename_folder(const char *folder_id, const char *new_name);

// File operations
bool db_create_file(const char *file_name, long file_size,
                    const char *folder_id, const char *user_name);
bool db_copy_file(const char *file_id, const char *to_folder_id);
bool db_move_file(const char *file_id, const char *to_folder_id);
bool db_rename_file(const char *file_id, const char *new_name);
bool db_delete_file(const char *file_id);

// Helper functions
FolderList *db_get_all_folder_in_folder(const char *folder_id);
FileList *db_get_all_file_in_folder(const char *folder_id);
bool db_check_folder_exist(const char *folder_name, const char *user_name,
                           const char *parent_folder_id);
bool db_check_file_exist(const char *file_name, const char *user_name,
                         const char *parent_folder_id);
FileList *db_search_file(const char *file_name);
FolderList *db_search_folder(const char *folder_name);

// Access operations
bool db_set_file_access(const char *file_id, const char *access,
                        const char *user_name);
bool db_set_folder_access(const char *folder_id, const char *access,
                          const char *user_name);
char *db_get_file_access(const char *file_id);
char *db_get_folder_access(const char *folder_id);

// Add to existing functions
char *db_get_file_id(const char *file_name, const char *parent_folder_id);

FileStruct *db_get_file_info(const char *file_id);

char *db_get_file_path(const char *file_id);
char *db_get_folder_path(const char *folder_id);

#endif  // DB_ACCESS_H