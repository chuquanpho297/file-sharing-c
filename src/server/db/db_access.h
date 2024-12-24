#ifndef DB_ACCESS_H
#define DB_ACCESS_H

#include <stdbool.h>
#include <mysql/mysql.h>
#include "../models/folder_model.h"
#include "../models/group_model.h"
#include "../models/user_model.h"
#include "../models/join_model.h"

// Database connection
MYSQL* db_connect(void);
void db_disconnect(MYSQL* conn);

// File operations
bool db_create_file(const char* file_name, long file_size, const char* folder_id);
bool db_delete_file(const char* file_name, const char* group_name, const char* folder_name);
bool db_rename_file(const char* file_name, const char* new_name, const char* group_name, const char* folder_name);
bool db_copy_file(const char* file_id, const char* to_folder_id);
bool db_move_file(const char* file_id, const char* to_folder_id);

// Folder operations
bool db_create_folder(const char* folder_name, const char* parent_folder_id);
bool db_delete_folder(const char* folder_name, const char* user_name);
bool db_rename_folder(const char* folder_id, const char* new_name);
bool db_copy_folder(const char* from_folder_id, const char* to_folder_id);
bool db_move_folder(const char* from_folder_id, const char* to_folder_id);
FolderContents* db_folder_contents(const char* group_name, const char* folder_name);


// User operations
bool db_create_user(const char* user_name, const char* password);
bool db_login(const char* user_name, const char* password);

// Helper functions to get IDs
char* db_get_folder_id(const char* folder_name, const char* user_name);
char* db_get_parent_folder_id(const char* folder_name, const char* user_name);

// Add this struct to hold search results
typedef struct {
    char* file_id;
    char* folder_id;
    char* file_name;
    long file_size;
    char* folder_name;
    char* created_by;
    char* access;
} FileSearchResult;

typedef struct {
    FileSearchResult* results;
    int count;
} FileSearchList;

// Add function declaration
FileSearchList* db_search_file(const char* file_name);
void file_search_list_destroy(FileSearchList* list);

#endif // DB_ACCESS_H 