#include "folder_controller.h"
#include "../db/db_access.h"
#include <stdlib.h>
#include <string.h>

bool create_root_folder_handler(const char* folder_name, const char* user_name) {
    return db_create_root_folder(folder_name, user_name);
}

bool create_folder_handler(const char* folder_name, const char* parent_folder_id) {
    return db_create_folder(folder_name, parent_folder_id);
}

char* get_root_folder_id_handler(const char* user_name) {
    return db_get_root_folder_id(user_name);
}

char* get_folder_id_handler(const char* folder_name, const char* user_name, const char* parent_folder_id) {
    return db_get_folder_id(folder_name, user_name, parent_folder_id);
}

bool copy_folder_handler(const char* folder_id, const char* parent_folder_id, const char* user_name) {
    return db_copy_folder(folder_id, parent_folder_id, user_name);
}

bool copy_all_content_folder_handler(const char* folder_id, const char* parent_folder_id) {
    return db_copy_all_content_folder(folder_id, parent_folder_id);
}

bool move_folder_handler(const char* folder_id, const char* parent_folder_id, const char* user_name) {
    return db_move_folder(folder_id, parent_folder_id, user_name);
}

bool rename_folder_handler(const char* folder_id, const char* new_name) {
    return db_rename_folder(folder_id, new_name);
}

bool delete_folder_handler(const char* folder_id) {
    return db_delete_folder(folder_id);
}

bool set_folder_access_handler(const char* folder_id, const char* access) {
    return db_set_folder_access(folder_id, access);
}

char* get_folder_access_handler(const char* folder_id) {
    return db_get_folder_access(folder_id);
}

FolderList* get_all_folders_in_folder_handler(const char* folder_id) {
    return db_get_all_folder_in_folder(folder_id);
}

FolderList* search_folders_handler(const char* folder_name) {
    return db_search_folder(folder_name);
}

bool check_folder_exists_handler(const char* folder_name, const char* user_name, const char* parent_folder_id) {
    return db_check_folder_exist(folder_name, user_name, parent_folder_id);
}
