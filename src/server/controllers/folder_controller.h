#ifndef FOLDER_CONTROLLER_H
#define FOLDER_CONTROLLER_H

#include <stdbool.h>
#include "../../utils/structs.h"

// Folder operations
bool create_root_folder_handler(const char* folder_name, const char* user_name);
bool create_folder_handler(const char* folder_name, const char* parent_folder_id);
char* get_root_folder_id_handler(const char* user_name);
char* get_folder_id_handler(const char* folder_name, const char* user_name, const char* parent_folder_id);
bool copy_folder_handler(const char* folder_id, const char* parent_folder_id, const char* user_name);
bool move_folder_handler(const char* folder_id, const char* parent_folder_id, const char* user_name);
bool rename_folder_handler(const char* folder_id, const char* new_name);
bool delete_folder_handler(const char* folder_id);

// Access control
bool set_folder_access_handler(const char* folder_id, const char* access);
char* get_folder_access_handler(const char* folder_id);

// Queries
FolderList* get_all_folders_in_folder_handler(const char* folder_id);
FolderList* search_folders_handler(const char* folder_name);
bool check_folder_exists_handler(const char* folder_name, const char* user_name, const char* parent_folder_id);

#endif // FOLDER_CONTROLLER_H 