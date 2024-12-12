#include "group_controller.h"
#include "../db/db_access.h"
#include <stdlib.h>
#include <string.h>

bool create_group(const char* user_name, const char* group_name) {
    return db_create_group(user_name, group_name);
}

bool is_admin(const char* user_name, const char* group_name) {
    return db_check_is_admin(user_name, group_name);
}

bool is_member(const char* user_name, const char* group_name) {
    return db_check_is_member(user_name, group_name);
}

bool remove_member(const char* member, const char* group_name) {
    return db_remove_member(member, group_name);
}

bool leave_group(const char* user_name, const char* group_name) {
    return db_remove_member(user_name, group_name);
}

bool invite_to_group(const char* invited_person, const char* group_name) {
    return db_invite_to_group(invited_person, group_name);
}

GroupMemberList* list_members(const char* group_name) {
    return db_list_members(group_name);
}

GroupList* list_all_groups(void) {
    return db_list_all_groups();
}

bool request_join(const char* user_name, const char* group_name) {
    return db_request_join(user_name, group_name);
}

bool accept_join(const char* user_name, const char* group_name) {
    return db_accept_join(user_name, group_name);
}

bool deny_join(const char* user_name, const char* group_name) {
    return db_deny_join(user_name, group_name);
} 