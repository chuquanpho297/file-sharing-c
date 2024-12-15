#define _XOPEN_SOURCE 500

#include "db_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "../models/join_model.h"
#include "../../utils/config.h"

static MYSQL *connection = NULL;

MYSQL *db_connect(void)
{
    if (connection != NULL)
    {
        return connection;
    }

    connection = mysql_init(NULL);
    if (connection == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        return NULL;
    }

    if (mysql_real_connect(connection, DB_HOST, DB_USER, DB_PASS, DB_NAME,
                           DB_PORT, NULL, 0) == NULL)
    {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(connection);
        return NULL;
    }

    return connection;
}

void db_disconnect(MYSQL *conn)
{
    if (conn != NULL)
    {
        mysql_close(conn);
        connection = NULL;
    }
}

bool db_create_file(const char *file_name, long file_size, const char *group_name, const char *folder_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT CreateFile('%s', %ld, '%s', '%s') AS Success",
             file_name, file_size, group_name, folder_name);

    if (mysql_query(conn, query))
    {
        printf("Create file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_delete_file(const char *file_name, const char *group_name, const char *folder_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT RemoveFile('%s', '%s', '%s') AS Success",
             file_name, group_name, folder_name);

    if (mysql_query(conn, query))
    {
        printf("Delete file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_create_folder(const char *folder_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
    {
        printf("Create folder failed: %s\n", mysql_error(conn));
        return false;
    }

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT CreateFolder('%s', '%s') AS Success",
             folder_name, group_name);

    if (mysql_query(conn, query))
    {
        printf("Create folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_delete_folder(const char *folder_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT RemoveFolder('%s', '%s') AS Success",
             folder_name, group_name);

    if (mysql_query(conn, query))
    {
        printf("Delete folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

FolderContents *db_folder_contents(const char *group_name, const char *folder_name)
{
    MYSQL *conn = db_connect();
    if (conn == NULL)
        return NULL;

    char query[1024];
    snprintf(query, sizeof(query),
             "SELECT fName, fileSize FROM File WHERE groupName='%s' AND folderName='%s'",
             group_name, folder_name);

    if (mysql_query(conn, query) != 0)
    {
        printf("Folder contents failed: %s\n", mysql_error(conn));
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL)
    {
        printf("Folder contents failed: %s\n", mysql_error(conn));
        return NULL;
    }

    FolderContents *contents = folder_contents_create();
    if (contents == NULL)
    {
        mysql_free_result(result);
        return NULL;
    }

    contents->file_count = mysql_num_rows(result);
    contents->files = malloc(contents->file_count * sizeof(FileInfo));
    contents->folder_name = strdup(folder_name);
    contents->group_name = strdup(group_name);

    int i = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
        contents->files[i].file_name = strdup(row[0]);
        contents->files[i].file_size = atol(row[1]);
        i++;
    }

    mysql_free_result(result);
    return contents;
}

bool db_request_join(const char *user_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (conn == NULL)
        return false;

    char query[1024];
    snprintf(query, sizeof(query),
             "SELECT RequestToJoinGroup('%s','%s') AS Success;",
             user_name, group_name);

    if (mysql_query(conn, query) != 0)
    {
        printf("Request join failed: %s\n", mysql_error(conn));
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL)
    {
        printf("Request join failed: %s\n", mysql_error(conn));
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && atoi(row[0]) == 1);

    mysql_free_result(result);
    return success;
}

bool db_accept_join(const char *user_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (conn == NULL)
        return false;

    char query[1024];
    snprintf(query, sizeof(query),
             "SELECT AcceptUsertoGroup('%s', '%s') AS Success",
             user_name, group_name);

    if (mysql_query(conn, query) != 0)
    {
        printf("Accept join failed: %s\n", mysql_error(conn));
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL)
    {
        printf("Accept join failed: %s\n", mysql_error(conn));
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && atoi(row[0]) == 1);

    mysql_free_result(result);
    return success;
}

JoinRequestList *db_list_join_requests(const char *group_name)
{
    MYSQL *conn = db_connect();
    if (conn == NULL)
        return NULL;

    char query[1024];
    snprintf(query, sizeof(query),
             "SELECT userName, createAt FROM `JoinGroup` WHERE status = 'pending' AND groupName = '%s;",
             group_name);

    if (mysql_query(conn, query) != 0)
    {
        printf("List join requests failed: %s\n", mysql_error(conn));
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL)
    {
        printf("List join requests failed: %s\n", mysql_error(conn));
        return NULL;
    }

    JoinRequestList *list = join_request_list_create();
    if (list == NULL)
    {
        mysql_free_result(result);
        return NULL;
    }

    list->request_count = mysql_num_rows(result);
    list->requests = malloc(list->request_count * sizeof(JoinRequest));

    int i = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
        list->requests[i].user_name = strdup(row[0]);

        // Convert timestamp string to time_t
        struct tm tm = {0};
        strptime(row[1], "%Y-%m-%d %H:%M:%S", &tm);
        list->requests[i].request_time = mktime(&tm);
        i++;
    }

    mysql_free_result(result);
    return list;
}

bool db_rename_file(const char *file_name, const char *new_name, const char *group_name, const char *folder_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT RenameFile('%s', '%s', '%s', '%s') AS Success",
             file_name, group_name, folder_name, new_name);

    if (mysql_query(conn, query))
    {
        printf("Rename file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_create_group(const char *user_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT CreateNewGroup('%s', '%s') AS Success",
             user_name, group_name);

    if (mysql_query(conn, query))
    {
        printf("Create group failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');
    db_disconnect(conn);
    return success;
}

bool db_check_is_admin(const char *user_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT CheckIsAdmin('%s', '%s') AS Success",
             group_name, user_name);

    if (mysql_query(conn, query))
    {
        printf("Check is admin failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');
    db_disconnect(conn);
    return success;
}

bool db_check_is_member(const char *user_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT CheckIsMember('%s', '%s') AS Success",
             user_name, group_name);

    if (mysql_query(conn, query))
    {
        printf("Check is member failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');
    db_disconnect(conn);
    return success;
}

bool db_remove_member(const char *member_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT LeaveGroup('%s','%s') AS Success;",
             member_name, group_name);

    if (mysql_query(conn, query))
    {
        printf("Remove member failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');
    db_disconnect(conn);
    return success;
}

bool db_invite_to_group(const char *invited_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT InviteToGroup('%s', '%s') AS Success",
             invited_name, group_name);

    if (mysql_query(conn, query))
    {
        printf("Invite to group failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');
    db_disconnect(conn);
    return success;
}

GroupMemberList *db_list_members(const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT userName, mRole FROM MemberOfGroup WHERE groupName='%s'", group_name);

    if (mysql_query(conn, query))
    {
        printf("List members failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    GroupMemberList *list = malloc(sizeof(GroupMemberList));
    list->member_count = mysql_num_rows(result);
    list->members = malloc(list->member_count * sizeof(GroupMember));

    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(result)))
    {
        list->members[i].user_name = strdup(row[0]);
        list->members[i].role = strdup(row[1]);
        i++;
    }

    mysql_free_result(result);
    db_disconnect(conn);
    return list;
}

bool db_create_user(const char *user_name, const char *password)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "Select InsertNewUser('%s', '%s') AS Success;",
             user_name, password);

    if (mysql_query(conn, query))
    {
        printf("Create user failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');
    db_disconnect(conn);
    return success;
}

bool db_login(const char *user_name, const char *password)
{
    MYSQL *conn = db_connect();
    if (!conn)
    {
        printf("Login failed: %s\n", mysql_error(conn));
        return false;
    }

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT Login('%s', '%s') AS Success;",
             user_name, password);
    if (mysql_query(conn, query))
    {
        printf("Login failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');
    db_disconnect(conn);
    return success;
}

GroupList *db_list_all_groups(void)
{
    MYSQL *conn = db_connect();
    if (!conn)
    {
        printf("List all groups failed: %s\n", mysql_error(conn));
        return NULL;
    }

    char query[512] = "SELECT groupName, createBy, createAt FROM `Groups`";

    if (mysql_query(conn, query))
    {
        printf("List all groups failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    GroupList *list = group_list_create();
    list->group_count = mysql_num_rows(result);
    list->groups = malloc(list->group_count * sizeof(Group));

    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(result)))
    {
        list->groups[i].group_name = strdup(row[0]);
        list->groups[i].created_by = strdup(row[1]);
        list->groups[i].created_at = strdup(row[2]);
        i++;
    }

    mysql_free_result(result);
    db_disconnect(conn);
    return list;
}

bool db_deny_join(const char *user_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT DenyUsertoGroup('%s','%s') AS Success;",
             user_name, group_name);

    if (mysql_query(conn, query))
    {
        printf("Deny join failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');
    db_disconnect(conn);
    return success;
}

JoinRequestStatus *db_get_join_status(const char *user_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT groupName, status, createAt FROM `JoinGroup` WHERE userName = '%s' and requestType = 'join';",
             user_name);

    if (mysql_query(conn, query))
    {
        printf("Get join status failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get join status failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    JoinRequestStatus *status = malloc(sizeof(JoinRequestStatus));
    if (row)
    {
        status->group_name = strdup(row[0]);
        status->status = strcmp(row[1], "pending") == 0 ? JOIN_STATUS_PENDING : strcmp(row[1], "accepted") == 0 ? JOIN_STATUS_ACCEPTED
                                                                                                                : JOIN_STATUS_DENIED;
        status->request_time = strdup(row[2]);
    }

    mysql_free_result(result);
    db_disconnect(conn);
    return status;
}

bool db_rename_folder(const char *group_name, const char *folder_name, const char *new_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT RenameFolder('%s', '%s', '%s') AS Success",
             group_name, folder_name, new_name);

    if (mysql_query(conn, query))
    {
        printf("Rename folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_copy_folder(const char *from_group, const char *folder_name, const char *to_group)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT CopyFolder('%s', '%s', '%s') AS Success",
             from_group, folder_name, to_group);

    if (mysql_query(conn, query))
    {
        printf("Copy folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_join_group(const char *user_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT RequestToJoinGroup('%s', '%s') AS Success",
             user_name, group_name);

    if (mysql_query(conn, query))
    {
        printf("Join group failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_leave_group(const char *user_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT LeaveGroup('%s', '%s') AS Success",
             user_name, group_name);

    if (mysql_query(conn, query))
    {
        printf("Leave group failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_invite_join(const char *user_name, const char *group_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT InviteToGroup('%s','%s') AS Success;",
             user_name, group_name);

    if (mysql_query(conn, query))
    {
        printf("Invite join failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

JoinRequestStatus *db_get_invitation_status(const char *user_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT groupName, status, createAt FROM `JoinGroup` WHERE userName = '%s' and requestType = 'invite';",
             user_name);

    if (mysql_query(conn, query))
    {
        printf("Get invitation status failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    JoinRequestStatus *status = malloc(sizeof(JoinRequestStatus));
    if (row)
    {
        status->group_name = strdup(row[0]);
        status->status = strcmp(row[1], "pending") == 0 ? JOIN_STATUS_PENDING : strcmp(row[1], "accepted") == 0 ? JOIN_STATUS_ACCEPTED
                                                                                                                : JOIN_STATUS_DENIED;
        status->request_time = strdup(row[2]);
    }

    mysql_free_result(result);
    db_disconnect(conn);
    return status;
}

// Similar implementations for other database functions...