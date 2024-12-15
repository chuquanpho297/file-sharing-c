#ifndef GROUP_CONTROLLER_H
#define GROUP_CONTROLLER_H

#include <stdbool.h>
#include "../models/group_model.h"

// Group operations
bool create_group(const char* user_name, const char* group_name);
bool is_admin(const char* user_name, const char* group_name);
bool is_member(const char* user_name, const char* group_name);
bool remove_member(const char* member, const char* group_name);
bool leave_group(const char* user_name, const char* group_name);
bool invite_to_group(const char* invited_person, const char* group_name);
GroupMemberList* list_members(const char* group_name);
GroupList* list_all_groups(void);
bool request_join(const char* user_name, const char* group_name);
bool accept_join(const char* user_name, const char* group_name);
bool deny_join(const char* user_name, const char* group_name);

#endif // GROUP_CONTROLLER_H 