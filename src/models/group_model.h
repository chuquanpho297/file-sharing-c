#ifndef GROUP_MODEL_H
#define GROUP_MODEL_H

typedef struct {
    char* user_name;
    char* role;
} GroupMember;

typedef struct {
    GroupMember* members;
    int member_count;
} GroupMemberList;

typedef struct {
    char* group_name;
    char* created_by;
    char* created_at;
} Group;

typedef struct {
    Group* groups;
    int group_count;
} GroupList;

// Constructor and destructor functions
GroupMemberList* group_member_list_create(void);
void group_member_list_destroy(GroupMemberList* list);

GroupList* group_list_create(void);
void group_list_destroy(GroupList* list);

#endif // GROUP_MODEL_H 