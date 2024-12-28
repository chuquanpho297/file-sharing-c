#include "db_access.h"

#include <mysql/mysql.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    if (conn != NULL && connection != NULL)
    {
        mysql_close(conn);
        connection = NULL;
    }
}

bool db_create_user(const char *user_name, const char *password)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "Select InsertNewUser('%s', '%s') AS Success;", user_name,
             password);

    if (mysql_query(conn, query))
    {
        printf("Create user failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Create user failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] && row[0][0] == '1');
    mysql_free_result(result);
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
    snprintf(query, sizeof(query), "SELECT Login('%s', '%s') AS Success;",
             user_name, password);
    if (mysql_query(conn, query))
    {
        printf("Login failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Login failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] && row[0][0] == '1');
    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_create_file(const char *file_name, long file_size,
                    const char *folder_id, const char *user_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT CreateFile('%s', %ld, '%s', '%s') AS file_id", file_name,
             file_size, folder_id, user_name);

    printf("Query: %s\n", query);

    if (mysql_query(conn, query))
    {
        printf("Create file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Create file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] != NULL);

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_create_folder(const char *folder_name, const char *parent_folder_id,
                      const char *user_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
    {
        printf("Create folder failed: %s\n", mysql_error(conn));
        return false;
    }

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT CreateFolder('%s', '%s', '%s') AS Success", folder_name,
             parent_folder_id, user_name);

    printf("Query: %s\n", query);

    if (mysql_query(conn, query))
    {
        printf("Create folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Create folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        printf("Create folder failed: %s\n", mysql_error(conn));
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

char *db_get_root_folder_id(const char *user_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query), "SELECT GetRootFolderID('%s') AS folder_id",
             user_name);

    if (mysql_query(conn, query))
    {
        printf("Get root folder ID failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get root folder ID failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return NULL;
    }
    char *folder_id = row && row[0] ? strdup(row[0]) : NULL;

    mysql_free_result(result);
    db_disconnect(conn);
    return folder_id;
}

FileList *db_get_all_file_in_folder(const char *folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query), "CALL GetAllFileInFolder('%s')", folder_id);

    if (mysql_query(conn, query))
    {
        printf("Get files failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get files failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    FileList *list = malloc(sizeof(FileList));
    list->count = mysql_num_rows(result);
    list->files = malloc(list->count * sizeof(FileStruct));

    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(result)))
    {
        list->files[i].file_id = strdup(row[0]);
        list->files[i].folder_id = strdup(row[1]);
        list->files[i].file_name = strdup(row[2]);
        list->files[i].file_size = atol(row[3]);
        i++;
    }

    mysql_free_result(result);
    db_disconnect(conn);
    return list;
}

bool db_create_root_folder(const char *user_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT CreateRootFolder('%s', '%s') AS Success", user_name,
             user_name);

    if (mysql_query(conn, query))
    {
        printf("Create root folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Create root folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

char *db_get_folder_id(const char *folder_name, const char *user_name,
                       const char *parent_folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT GetFolderID('%s', '%s', '%s') AS folder_id", folder_name,
             user_name, parent_folder_id);
    printf("Query: %s\n", query);
    if (mysql_query(conn, query))
    {
        printf("Get folder ID failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get folder ID failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return NULL;
    }
    char *folder_id = row && row[0] ? strdup(row[0]) : NULL;
    printf("Folder ID: %s\n", folder_id);
    mysql_free_result(result);
    db_disconnect(conn);
    return folder_id;
}

char *db_get_parent_folder_id(const char *folder_name, const char *user_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT parentFolderID FROM Folder WHERE folderName='%s' AND "
             "createBy='%s'",
             folder_name, user_name);

    if (mysql_query(conn, query))
    {
        printf("Get parent folder ID failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get parent folder ID failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return NULL;
    }
    char *parent_id = row && row[0] ? strdup(row[0]) : NULL;

    mysql_free_result(result);
    db_disconnect(conn);
    return parent_id;
}

bool db_copy_folder(const char *from_folder_id, const char *to_folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    // Retrieve properties of from_folder_id
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT folderName, createBy FROM Folder WHERE folderID = '%s'",
             from_folder_id);

    printf("Query: %s\n", query);

    if (mysql_query(conn, query))
    {
        printf("Create new folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result1 = mysql_store_result(conn);
    if (!result1)
    {
        printf("Create new folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result1);
    if (!row)
    {
        mysql_free_result(result1);
        db_disconnect(conn);
        return false;
    }

    char *folder_name = row[0];
    char *create_by = row[1];
    // Create new folder with the same properties but with to_folder_id as
    // parent
    snprintf(query, sizeof(query),
             "INSERT INTO Folder (folderID, folderName, parentFolderID, "
             "createBy, createAt) "
             "VALUES (UUID(), '%s', '%s', '%s', NOW())",
             folder_name, to_folder_id, create_by);

    printf("Query: %s\n", query);

    if (mysql_query(conn, query))
    {
        printf("Create new folder failed: %s\n", mysql_error(conn));
        mysql_free_result(result1);
        db_disconnect(conn);
        return false;
    }

    snprintf(query, sizeof(query),
             "SELECT folderID FROM Folder WHERE "
             "folderName = '%s' AND parentFolderID = '%s'",
             folder_name, to_folder_id);

    printf("Query: %s\n", query);

    if (mysql_query(conn, query))
    {
        printf("Create new folder failed: %s\n", mysql_error(conn));
        mysql_free_result(result1);
        db_disconnect(conn);
        return false;
    }

    mysql_free_result(result1);

    MYSQL_RES *result2 = mysql_store_result(conn);
    if (!result2)
    {
        printf("Create new folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    row = mysql_fetch_row(result2);
    if (!row)
    {
        mysql_free_result(result2);
        db_disconnect(conn);
        return false;
    }

    const char *new_folder_id = row[0];
    printf("New folder ID: %s\n", new_folder_id);

    if (!db_copy_all_content_folder(from_folder_id, new_folder_id))
    {
        mysql_free_result(result2);
        db_disconnect(conn);
        return false;
    }

    mysql_free_result(result2);
    db_disconnect(conn);
    return true;
}

bool db_copy_all_content_folder(const char *from_folder_id,
                                const char *to_folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    bool success = true;  // Track overall success
    MYSQL_RES *result = NULL;
    MYSQL_RES *result2 = NULL;
    MYSQL_RES *result3 = NULL;

    // First copy all files from source folder to destination folder
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT fName, fileSize, createBy FROM File WHERE folderID = '%s'",
             from_folder_id);

    printf("Query: %s\n", query);

    if (mysql_query(conn, query))
    {
        printf("Get files failed: %s\n", mysql_error(conn));
        success = false;
        goto cleanup;
    }

    result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get files failed: %s\n", mysql_error(conn));
        success = false;
        goto cleanup;
    }

    // Copy each file
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
        char insert_query[512];
        snprintf(insert_query, sizeof(insert_query),
                 "INSERT INTO File (fileID, folderID, fName, fileSize, "
                 "createBy, createAt) "
                 "VALUES (UUID(), '%s', '%s', %s, '%s', NOW())",
                 to_folder_id, row[0], row[1], row[2]);

        printf("Query: %s\n", insert_query);

        if (mysql_query(conn, insert_query))
        {
            printf("Copy file failed: %s\n", mysql_error(conn));
            success = false;
            goto cleanup;
        }
    }

    // Now handle subfolders
    snprintf(query, sizeof(query),
             "SELECT folderID, folderName, createBy FROM Folder WHERE "
             "parentFolderID = '%s'",
             from_folder_id);

    printf("Query: %s\n", query);

    if (mysql_query(conn, query))
    {
        printf("Get subfolders failed: %s\n", mysql_error(conn));
        success = false;
        goto cleanup;
    }

    result2 = mysql_store_result(conn);
    if (!result2)
    {
        printf("Get subfolders failed: %s\n", mysql_error(conn));
        success = false;
        goto cleanup;
    }

    // If there are no subfolders, we're done
    if (mysql_num_rows(result2) == 0)
    {
        printf("No subfolders found, copy complete\n");
        goto cleanup;  // This is a successful case
    }

    // Process each subfolder
    while ((row = mysql_fetch_row(result2)))
    {
        // Create new subfolder
        char create_folder_query[512];
        snprintf(create_folder_query, sizeof(create_folder_query),
                 "INSERT INTO Folder (folderID, folderName, parentFolderID, "
                 "createBy, createAt) "
                 "VALUES (UUID(), '%s', '%s', '%s', NOW())",
                 row[1], to_folder_id, row[2]);

        printf("Query: %s\n", create_folder_query);

        if (mysql_query(conn, create_folder_query))
        {
            printf("Create subfolder failed: %s\n", mysql_error(conn));
            success = false;
            goto cleanup;
        }

        // Get the new folder's ID
        char get_new_folder_query[512];
        snprintf(get_new_folder_query, sizeof(get_new_folder_query),
                 "SELECT folderID FROM Folder WHERE folderName = '%s' AND "
                 "parentFolderID = '%s' "
                 "ORDER BY createAt DESC LIMIT 1",
                 row[1], to_folder_id);

        printf("Query: %s\n", get_new_folder_query);

        if (mysql_query(conn, get_new_folder_query))
        {
            printf("Get new folder ID failed: %s\n", mysql_error(conn));
            success = false;
            goto cleanup;
        }

        result3 = mysql_store_result(conn);
        if (!result3)
        {
            printf("Get new folder ID failed: %s\n", mysql_error(conn));
            success = false;
            goto cleanup;
        }

        MYSQL_ROW id_row = mysql_fetch_row(result3);
        if (!id_row || !id_row[0])
        {
            printf("Failed to get new folder ID\n");
            success = false;
            goto cleanup;
        }

        // Recursively copy contents of subfolder
        char *new_folder_id = strdup(id_row[0]);
        mysql_free_result(result3);
        result3 = NULL;

        if (!new_folder_id)
        {
            printf("Memory allocation failed\n");
            success = false;
            goto cleanup;
        }

        bool copy_success = db_copy_all_content_folder(row[0], new_folder_id);
        free(new_folder_id);

        if (!copy_success)
        {
            success = false;
            goto cleanup;
        }
    }

cleanup:
    if (result3)
        mysql_free_result(result3);
    if (result2)
        mysql_free_result(result2);
    if (result)
        mysql_free_result(result);
    return success;
}

bool db_move_all_content_folder(const char *from_folder_id,
                                const char *to_folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT MoveAllContentFolder('%s', '%s') AS Success",
             from_folder_id, to_folder_id);

    if (mysql_query(conn, query))
    {
        printf("Move folder contents failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Move folder contents failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_move_folder(const char *folder_id, const char *parent_folder_id,
                    const char *user_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT MoveFolder('%s', '%s', '%s') AS Success", folder_id,
             parent_folder_id, user_name);

    if (mysql_query(conn, query))
    {
        printf("Move folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Move folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_delete_folder(const char *folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    // First, recursively get and delete all subfolders
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT folderID FROM Folder WHERE parentFolderID = '%s'",
             folder_id);

    printf("Query: %s\n", query);

    if (mysql_query(conn, query))
    {
        printf("Get subfolders failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get subfolders failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    // Recursively delete all subfolders
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
        if (!db_delete_folder(row[0]))
        {
            mysql_free_result(result);
            db_disconnect(conn);
            return false;
        }
    }
    mysql_free_result(result);

    // Delete all files in the current folder
    snprintf(query, sizeof(query), "DELETE FROM File WHERE folderID = '%s'",
             folder_id);

    if (mysql_query(conn, query))
    {
        printf("Delete files failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    // Delete the folder itself
    snprintf(query, sizeof(query), "DELETE FROM Folder WHERE folderID = '%s'",
             folder_id);

    if (mysql_query(conn, query))
    {
        printf("Delete folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    db_disconnect(conn);
    return true;
}

bool db_rename_folder(const char *folder_id, const char *new_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query), "SELECT RenameFolder('%s', '%s') AS Success",
             folder_id, new_name);

    if (mysql_query(conn, query))
    {
        printf("Rename folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Rename folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

FolderList *db_get_all_folder_in_folder(const char *folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query), "CALL GetAllFolderInFolder('%s')",
             folder_id);

    if (mysql_query(conn, query))
    {
        printf("Get folders failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get folders failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    FolderList *list = malloc(sizeof(FolderList));
    list->count = mysql_num_rows(result);
    list->folders = malloc(list->count * sizeof(FolderStruct));

    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(result)))
    {
        list->folders[i].folder_id = strdup(row[0]);
        list->folders[i].folder_name = strdup(row[1]);
        list->folders[i].created_by = strdup(row[2]);
        list->folders[i].parent_folder_id = row[3] ? strdup(row[3]) : NULL;
        list->folders[i].created_at = strdup(row[4]);
        list->folders[i].access = strdup(row[5]);
        i++;
    }

    mysql_free_result(result);
    db_disconnect(conn);
    return list;
}

bool db_check_folder_exist(const char *folder_name, const char *user_name,
                           const char *parent_folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT CheckFolderExist('%s', '%s', '%s') AS CheckFolderExist",
             folder_name, user_name, parent_folder_id);
    printf("Query: %s\n", query);
    if (mysql_query(conn, query))
    {
        printf("Check folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Check folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool exists = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return exists;
}

bool db_check_file_exist(const char *file_name, const char *user_name,
                         const char *parent_folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT CheckFileExists('%s', '%s', '%s') AS result", file_name,
             user_name, parent_folder_id);

    if (mysql_query(conn, query))
    {
        printf("Check file failed: %s\n", mysql_error(conn));
        printf("Query: %s\n", query);
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Check file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool exists = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return exists;
}

FileList *db_search_file(const char *file_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query), "CALL SearchFile('%s')", file_name);

    if (mysql_query(conn, query))
    {
        printf("Search file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Search file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    FileList *list = malloc(sizeof(FileList));
    list->count = mysql_num_rows(result);
    list->files = malloc(list->count * sizeof(FileStruct));

    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(result)))
    {
        list->files[i].file_id = strdup(row[0]);
        list->files[i].folder_id = strdup(row[1]);
        list->files[i].file_name = strdup(row[2]);
        list->files[i].file_size = atol(row[3]);
        list->files[i].folder_name = strdup(row[4]);
        list->files[i].created_by = strdup(row[5]);
        list->files[i].created_at = strdup(row[6]);
        list->files[i].access = strdup(row[7]);
        list->files[i].file_path = strdup(row[8]);
        i++;
    }

    mysql_free_result(result);
    db_disconnect(conn);
    return list;
}

FolderList *db_search_folder(const char *folder_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query), "CALL SearchFolder('%s')", folder_name);

    if (mysql_query(conn, query))
    {
        printf("Search folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Search folder failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    FolderList *list = malloc(sizeof(FolderList));
    list->count = mysql_num_rows(result);
    list->folders = malloc(list->count * sizeof(FolderStruct));

    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(result)))
    {
        list->folders[i].folder_id = strdup(row[0]);
        list->folders[i].folder_name = strdup(row[1]);
        list->folders[i].created_by = strdup(row[2]);
        list->folders[i].access = strdup(row[3]);
        list->folders[i].folder_path = strdup(row[4]);
        i++;
    }

    mysql_free_result(result);
    db_disconnect(conn);
    return list;
}

bool db_set_file_access(const char *file_id, const char *access,
                        const char *user_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT SetFileAccess('%s', '%s', '%s') AS Success", file_id,
             access, user_name);

    if (mysql_query(conn, query))
    {
        printf("Set file access failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Set file access failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_set_folder_access(const char *folder_id, const char *access,
                          const char *user_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT SetFolderAccess('%s', '%s', '%s') AS Success", folder_id,
             access, user_name);

    if (mysql_query(conn, query))
    {
        printf("Set folder access failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Set folder access failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

char *db_get_file_access(const char *file_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query), "SELECT GetFileAccess('%s') AS access",
             file_id);

    if (mysql_query(conn, query))
    {
        printf("Get file access failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get file access failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return NULL;
    }
    char *access = row && row[0] ? strdup(row[0]) : NULL;
    printf("Access: %s\n", access);
    mysql_free_result(result);
    db_disconnect(conn);
    return access;
}

char *db_get_folder_access(const char *folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query), "SELECT GetFolderAccess('%s') AS access",
             folder_id);

    if (mysql_query(conn, query))
    {
        printf("Get folder access failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get folder access failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return NULL;
    }
    char *access = row && row[0] ? strdup(row[0]) : NULL;

    mysql_free_result(result);
    db_disconnect(conn);
    return access;
}

bool db_copy_file(const char *file_id, const char *to_folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query), "SELECT CopyFile('%s', '%s') AS new_file_id",
             file_id, to_folder_id);

    if (mysql_query(conn, query))
    {
        printf("Copy file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Copy file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] != NULL);

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_move_file(const char *file_id, const char *to_folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query), "SELECT MoveFile('%s', '%s') AS Success",
             file_id, to_folder_id);

    if (mysql_query(conn, query))
    {
        printf("Move file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Move file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_rename_file(const char *file_id, const char *new_name)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query), "SELECT RenameFile('%s', '%s') AS Success",
             file_id, new_name);

    if (mysql_query(conn, query))
    {
        printf("Rename file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Rename file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

bool db_delete_file(const char *file_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return false;

    char query[512];
    snprintf(query, sizeof(query), "SELECT DeleteFile('%s') AS Success",
             file_id);

    if (mysql_query(conn, query))
    {
        printf("Delete file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Delete file failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return false;
    }
    bool success = (row && row[0] && row[0][0] == '1');

    mysql_free_result(result);
    db_disconnect(conn);
    return success;
}

char *db_get_file_id(const char *file_name, const char *parent_folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query), "SELECT GetFileID('%s', '%s') AS file_id",
             file_name, parent_folder_id);

    if (mysql_query(conn, query))
    {
        printf("Get file ID failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get file ID failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return NULL;
    }
    char *file_id = row && row[0] ? strdup(row[0]) : NULL;

    mysql_free_result(result);
    db_disconnect(conn);
    return file_id;
}

FileStruct *db_get_file_info(const char *file_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT fileSize FROM File WHERE fileId = '%s'", file_id);

    if (mysql_query(conn, query))
    {
        printf("Get file info failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);

    if (!result)
    {
        printf("Get file info failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    if (!row)
    {
        printf("Get file info failed: %s\n", mysql_error(conn));
        mysql_free_result(result);
        db_disconnect(conn);
        return NULL;
    }

    FileStruct *file = malloc(sizeof(FileStruct));

    file->file_id = strdup(file_id);
    file->file_size = atol(row[0]);

    printf("File size: %ld\n", file->file_size);

    mysql_free_result(result);

    db_disconnect(conn);

    return file;
}

char *db_get_folder_path(const char *folder_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query), "SELECT GetFolderPath('%s') AS path",
             folder_id);

    if (mysql_query(conn, query))
    {
        printf("Get folder path failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get folder path failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return NULL;
    }
    char *path = row && row[0] ? strdup(row[0]) : NULL;

    mysql_free_result(result);
    db_disconnect(conn);
    return path;
}

char *db_get_file_path(const char *file_id)
{
    MYSQL *conn = db_connect();
    if (!conn)
        return NULL;

    char query[512];
    snprintf(query, sizeof(query), "SELECT GetFilePath('%s') AS path", file_id);

    if (mysql_query(conn, query))
    {
        printf("Get file path failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result)
    {
        printf("Get file path failed: %s\n", mysql_error(conn));
        db_disconnect(conn);
        return NULL;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row)
    {
        mysql_free_result(result);
        db_disconnect(conn);
        return NULL;
    }
    char *path = row && row[0] ? strdup(row[0]) : NULL;

    mysql_free_result(result);
    db_disconnect(conn);
    return path;
}