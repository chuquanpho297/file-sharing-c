#ifndef JOIN_MODEL_H
#define JOIN_MODEL_H

#include <time.h>

typedef enum {
    JOIN_STATUS_PENDING,
    JOIN_STATUS_ACCEPTED,
    JOIN_STATUS_DENIED
} JoinStatus;

typedef enum {
    REQUEST_TYPE_JOIN,
    REQUEST_TYPE_INVITE
} RequestType;

typedef struct {
    char* user_name;
    char* group_name;
    time_t request_time;
    JoinStatus status;
    RequestType type;
} JoinRequest;

typedef struct {
    JoinRequest* requests;
    int request_count;
} JoinRequestList;

typedef struct {
    char* group_name;
    JoinStatus status;
    char* request_time;
} JoinRequestStatus;

// Constructor and destructor functions
JoinRequestList* join_request_list_create(void);
void join_request_list_destroy(JoinRequestList* list);

JoinRequestStatus* join_request_status_create(void);
void join_request_status_destroy(JoinRequestStatus* status);

#endif // JOIN_MODEL_H 