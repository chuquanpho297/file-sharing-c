#include "group_model.h"
#include <stdlib.h>
#include <string.h>

GroupMemberList* group_member_list_create(void) {
    GroupMemberList* list = malloc(sizeof(GroupMemberList));
    if (list == NULL) return NULL;
    
    list->members = NULL;
    list->member_count = 0;
    return list;
}

void group_member_list_destroy(GroupMemberList* list) {
    if (list == NULL) return;
    
    for (int i = 0; i < list->member_count; i++) {
        free(list->members[i].user_name);
        free(list->members[i].role);
    }
    free(list->members);
    free(list);
}

GroupList* group_list_create(void) {
    GroupList* list = malloc(sizeof(GroupList));
    if (list == NULL) return NULL;
    
    list->groups = NULL;
    list->group_count = 0;
    return list;
}

void group_list_destroy(GroupList* list) {
    if (list == NULL) return;
    
    for (int i = 0; i < list->group_count; i++) {
        free(list->groups[i].group_name);
        free(list->groups[i].created_by);
        free(list->groups[i].created_at);
    }
    free(list->groups);
    free(list);
} 