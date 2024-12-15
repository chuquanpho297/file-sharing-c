#include "folder_controller.h"
#include "../db/db_access.h"
#include <stdlib.h>
#include <string.h>

bool create_folder(const char* folder_name, const char* group_name) {
    return db_create_folder(folder_name, group_name);
}

bool rename_folder(const char* group_name, const char* folder_name, const char* new_folder_name) {
    return db_rename_folder(group_name, folder_name, new_folder_name);
}

bool delete_folder(const char* group_name, const char* folder_name) {
    return db_delete_folder(folder_name, group_name);
}

bool move_folder(const char* from_group, const char* to_group, const char* folder_name) {
    if (db_create_folder(folder_name, to_group) && 
        db_delete_folder(folder_name, from_group)) {
        return true;
    }
    return false;
}

bool copy_folder(const char* from_group, const char* to_group, const char* folder_name) {
    return db_copy_folder(from_group, folder_name, to_group);
}

FolderContents* get_folder_contents(const char* group_name, const char* folder_name) {
    return db_folder_contents(group_name, folder_name);
} 