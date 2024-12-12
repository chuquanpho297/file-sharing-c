#include "file_controller.h"
#include "../db/db_access.h"
#include <stdlib.h>
#include <string.h>

bool create_file(const char* file_name, long file_size, const char* group_name, const char* folder_name) {
    return db_create_file(file_name, file_size, group_name, folder_name);
}

bool delete_file(const char* file_name, const char* group_name, const char* folder_name) {
    return db_delete_file(file_name, group_name, folder_name);
}

bool rename_file(const char* file_name, const char* new_file_name, const char* group_name, const char* folder_name) {
    return db_rename_file(file_name, new_file_name, group_name, folder_name);
}

bool move_file(const char* file_name, long file_size, const char* from_group, const char* to_group,
               const char* from_folder, const char* to_folder) {
    if (db_create_file(file_name, file_size, to_group, to_folder) && 
        db_delete_file(file_name, from_group, from_folder)) {
        return true;
    }
    return false;
}

bool copy_file(const char* file_name, long file_size, const char* from_group, const char* to_group,
               const char* from_folder, const char* to_folder) {
    return db_create_file(file_name, file_size, to_group, to_folder);
} 