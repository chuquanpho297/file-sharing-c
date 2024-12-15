#include "join_model.h"
#include <stdlib.h>
#include <string.h>

JoinRequestList* join_request_list_create(void) {
    JoinRequestList* list = malloc(sizeof(JoinRequestList));
    if (list == NULL) return NULL;
    
    list->requests = NULL;
    list->request_count = 0;
    return list;
}

void join_request_list_destroy(JoinRequestList* list) {
    if (list == NULL) return;
    
    for (int i = 0; i < list->request_count; i++) {
        free(list->requests[i].user_name);
        free(list->requests[i].group_name);
    }
    free(list->requests);
    free(list);
}

JoinRequestStatus* join_request_status_create(void) {
    JoinRequestStatus* status = malloc(sizeof(JoinRequestStatus));
    if (status == NULL) return NULL;
    
    status->group_name = NULL;
    status->status = JOIN_STATUS_PENDING;
    status->request_time = time(NULL);
    return status;
}

void join_request_status_destroy(JoinRequestStatus* status) {
    if (status == NULL) return;
    
    free(status->group_name);
    free(status);
} 