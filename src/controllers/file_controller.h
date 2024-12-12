#ifndef FILE_CONTROLLER_H
#define FILE_CONTROLLER_H

#include <stdbool.h>

// File operations
bool create_file(const char* file_name, long file_size, const char* group_name, const char* folder_name);
bool delete_file(const char* file_name, const char* group_name, const char* folder_name);
bool rename_file(const char* file_name, const char* new_file_name, const char* group_name, const char* folder_name);
bool move_file(const char* file_name, long file_size, const char* from_group, const char* to_group, 
               const char* from_folder, const char* to_folder);
bool copy_file(const char* file_name, long file_size, const char* from_group, const char* to_group,
               const char* from_folder, const char* to_folder);

#endif // FILE_CONTROLLER_H 