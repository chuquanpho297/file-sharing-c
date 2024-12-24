#ifndef FILE_CONTROLLER_H
#define FILE_CONTROLLER_H

#include <stdbool.h>
#include "../../utils/structs.h"

// File operations
bool create_file(const char* file_name, long file_size, const char* folder_id, const char* user_name);
bool copy_file(const char* file_id, const char* to_folder_id);
bool move_file(const char* file_id, const char* to_folder_id);
bool rename_file(const char* file_id, const char* new_name);
bool delete_file(const char* file_id);

// Access control
bool set_file_access(const char* file_id, const char* access);
char* get_file_access(const char* file_id);
char* get_file_id(const char* file_name, const char* parent_folder_id);

// Queries
FileList* get_all_files_in_folder(const char* folder_id);
FileList* search_files(const char* file_name);
bool check_file_exists(const char* file_name, const char* user_name, const char* parent_folder_id);

#endif // FILE_CONTROLLER_H 