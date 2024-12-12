#define _XOPEN_SOURCE 500

#include "db_access.h"
#include "../utils/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../models/join_model.h"

static MYSQL* connection = NULL;

MYSQL* db_connect(void) {
    if (connection != NULL) {
        return connection;
    }

    connection = mysql_init(NULL);
    if (connection == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return NULL;
    }

    if (mysql_real_connect(connection, DB_HOST, DB_USER, DB_PASS, DB_NAME, 
                          DB_PORT, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(connection);
        return NULL;
    }

    return connection;
}

void db_disconnect(MYSQL* conn) {
    if (conn != NULL) {
        mysql_close(conn);
        connection = NULL;
    }
}

bool db_create_file(const char* file_name, long file_size, const char* group_name, const char* folder_name) {
    MYSQL* conn = db_connect();
    if (conn == NULL) return false;

    char query[1024];
    snprintf(query, sizeof(query), 
             "SELECT CreateFile('%s', %ld, '%s', '%s') AS Success",
             file_name, file_size, group_name, folder_name);

    if (mysql_query(conn, query) != 0) {
        return false;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && atoi(row[0]) == 1);
    
    mysql_free_result(result);
    return success;
}

bool db_delete_file(const char* file_name, const char* group_name, const char* folder_name) {
    MYSQL* conn = db_connect();
    if (conn == NULL) return false;

    char query[1024];
    snprintf(query, sizeof(query), 
             "SELECT RemoveFile('%s', '%s', '%s') AS Success",
             file_name, group_name, folder_name);

    if (mysql_query(conn, query) != 0) {
        return false;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && atoi(row[0]) == 1);
    
    mysql_free_result(result);
    return success;
}

bool db_create_folder(const char* folder_name, const char* group_name) {
    MYSQL* conn = db_connect();
    if (conn == NULL) return false;

    char query[1024];
    snprintf(query, sizeof(query), 
             "SELECT CreateFolder('%s', '%s') AS Success",
             folder_name, group_name);

    if (mysql_query(conn, query) != 0) {
        return false;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && atoi(row[0]) == 1);
    
    mysql_free_result(result);
    return success;
}

FolderContents* db_folder_contents(const char* group_name, const char* folder_name) {
    MYSQL* conn = db_connect();
    if (conn == NULL) return NULL;

    char query[1024];
    snprintf(query, sizeof(query), 
             "SELECT fName, fileSize FROM File WHERE groupName='%s' AND folderName='%s'",
             group_name, folder_name);

    if (mysql_query(conn, query) != 0) {
        return NULL;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return NULL;
    }

    FolderContents* contents = folder_contents_create();
    if (contents == NULL) {
        mysql_free_result(result);
        return NULL;
    }

    contents->file_count = mysql_num_rows(result);
    contents->files = malloc(contents->file_count * sizeof(FileInfo));
    contents->folder_name = strdup(folder_name);
    contents->group_name = strdup(group_name);

    int i = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        contents->files[i].file_name = strdup(row[0]);
        contents->files[i].file_size = atol(row[1]);
        i++;
    }

    mysql_free_result(result);
    return contents;
}

bool db_request_join(const char* user_name, const char* group_name) {
    MYSQL* conn = db_connect();
    if (conn == NULL) return false;

    char query[1024];
    snprintf(query, sizeof(query), 
             "SELECT JoinRequest('%s', '%s') AS Success",
             user_name, group_name);

    if (mysql_query(conn, query) != 0) {
        return false;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && atoi(row[0]) == 1);
    
    mysql_free_result(result);
    return success;
}

bool db_accept_join(const char* user_name, const char* group_name) {
    MYSQL* conn = db_connect();
    if (conn == NULL) return false;

    char query[1024];
    snprintf(query, sizeof(query), 
             "SELECT AcceptJoinRequest('%s', '%s') AS Success",
             user_name, group_name);

    if (mysql_query(conn, query) != 0) {
        return false;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && atoi(row[0]) == 1);
    
    mysql_free_result(result);
    return success;
}

JoinRequestList* db_list_join_requests(const char* group_name) {
    MYSQL* conn = db_connect();
    if (conn == NULL) return NULL;

    char query[1024];
    snprintf(query, sizeof(query), 
             "SELECT userName, requestType, createAt FROM JoinGroup "
             "WHERE groupName='%s' AND status='pending'",
             group_name);

    if (mysql_query(conn, query) != 0) {
        return NULL;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (result == NULL) {
        return NULL;
    }

    JoinRequestList* list = join_request_list_create();
    if (list == NULL) {
        mysql_free_result(result);
        return NULL;
    }

    list->request_count = mysql_num_rows(result);
    list->requests = malloc(list->request_count * sizeof(JoinRequest));

    int i = 0;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        list->requests[i].user_name = strdup(row[0]);
        list->requests[i].group_name = strdup(group_name);
        list->requests[i].type = strcmp(row[1], "join") == 0 ? 
                                REQUEST_TYPE_JOIN : REQUEST_TYPE_INVITE;
        // Convert timestamp string to time_t
        struct tm tm = {0};
        strptime(row[2], "%Y-%m-%d %H:%M:%S", &tm);
        list->requests[i].request_time = mktime(&tm);
        i++;
    }

    mysql_free_result(result);
    return list;
}

bool db_rename_file(const char* file_name, const char* new_name, const char* group_name, const char* folder_name) {
    MYSQL* conn = db_connect();
    if (!conn) return false;

    char query[512];
    snprintf(query, sizeof(query), 
        "SELECT RenameFile('%s', '%s', '%s', '%s') AS Success;",
        file_name, group_name, folder_name, new_name);

    if (mysql_query(conn, query)) {
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

// Similar implementations for other database functions... 