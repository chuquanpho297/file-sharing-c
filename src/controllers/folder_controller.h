#ifndef FOLDER_CONTROLLER_H
#define FOLDER_CONTROLLER_H

#include <stdbool.h>
#include "../models/folder_model.h"

// Folder operations
bool create_folder(const char* folder_name, const char* group_name);
bool rename_folder(const char* group_name, const char* folder_name, const char* new_folder_name);
bool delete_folder(const char* group_name, const char* folder_name);
bool move_folder(const char* from_group, const char* to_group, const char* folder_name);
bool copy_folder(const char* from_group, const char* to_group, const char* folder_name);
FolderContents* get_folder_contents(const char* group_name, const char* folder_name);

#endif // FOLDER_CONTROLLER_H 