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
bool db_create_file(const char* file_name, long file_size, const char* group_name, const char* folder_name);
bool db_delete_file(const char* file_name, const char* group_name, const char* folder_name);
bool db_rename_file(const char* file_name, const char* new_name, const char* group_name, const char* folder_name);

// Folder operations
bool db_create_folder(const char* folder_name, const char* group_name);
bool db_delete_folder(const char* folder_name, const char* group_name);
bool db_rename_folder(const char* group_name, const char* folder_name, const char* new_name);
bool db_copy_folder(const char* from_group, const char* folder_name, const char* to_group);
FolderContents* db_folder_contents(const char* group_name, const char* folder_name);

// Group operations
bool db_create_group(const char* user_name, const char* group_name);
bool db_check_is_admin(const char* user_name, const char* group_name);
bool db_check_is_member(const char* user_name, const char* group_name);
bool db_remove_member(const char* member_name, const char* group_name);
bool db_invite_to_group(const char* invited_name, const char* group_name);
GroupMemberList* db_list_members(const char* group_name);
GroupList* db_list_all_groups(void);

// User operations
bool db_create_user(const char* user_name, const char* password);
bool db_login(const char* user_name, const char* password);

// Join operations
bool db_request_join(const char* user_name, const char* group_name);
bool db_accept_join(const char* user_name, const char* group_name);
bool db_deny_join(const char* user_name, const char* group_name);
JoinRequestList* db_list_join_requests(const char* group_name);
JoinRequestStatus* db_get_join_status(const char* user_name, const char* group_name);

#endif // DB_ACCESS_H 