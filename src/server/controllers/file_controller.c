#include "file_controller.h"
#include "../db/db_access.h"
#include <stdlib.h>
#include <string.h>

bool create_file(const char* file_name, long file_size, const char* folder_id, const char* user_name) {
    return db_create_file(file_name, file_size, folder_id, user_name);
}

bool copy_file(const char* file_id, const char* to_folder_id) {
    return db_copy_file(file_id, to_folder_id);
}

bool move_file(const char* file_id, const char* to_folder_id) {
    return db_move_file(file_id, to_folder_id);
}

bool rename_file(const char* file_id, const char* new_name) {
    return db_rename_file(file_id, new_name);
}

bool delete_file(const char* file_id) {
    return db_delete_file(file_id);
}

bool set_file_access(const char* file_id, const char* access) {
    return db_set_file_access(file_id, access);
}

char* get_file_access(const char* file_id) {
    return db_get_file_access(file_id);
}

FileList* get_all_files_in_folder(const char* folder_id) {
    return db_get_all_file_in_folder(folder_id);
}

FileList* search_files(const char* file_name) {
    return db_search_file(file_name);
}

bool check_file_exists(const char* file_name, const char* user_name, const char* parent_folder_id) {
    return db_check_file_exist(file_name, user_name, parent_folder_id);
}

char* get_file_id(const char* file_name, const char* parent_folder_id) {
    return db_get_file_id(file_name, parent_folder_id);
}

