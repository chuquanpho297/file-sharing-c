#include "folder_handler.h"
#include <arpa/inet.h>
#include <json-c/json.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../utils/helper.h"
#include "../../utils/structs.h"
#include "../db/db_access.h"

void handle_folder_content(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_id_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderId", &folder_id_obj);

    const char *folder_id = json_object_get_string(folder_id_obj);

    FolderList *folders = db_get_all_folder_in_folder(folder_id);
    FileList *files = db_get_all_file_in_folder(folder_id);

    if (!folders || !files)
    {
        send_response(client->socket, 500, "Failed to get folder contents");
        json_object_put(parsed_json);
        return;
    }

    struct json_object *response = json_object_new_object();
    struct json_object *resp_payload = json_object_new_object();
    struct json_object *folders_array = json_object_new_array();
    struct json_object *files_array = json_object_new_array();

    // Add folders to response
    for (int i = 0; i < folders->count; i++)
    {
        struct json_object *folder = json_object_new_object();
        json_object_object_add(
            folder, "folderId",
            json_object_new_string(folders->folders[i].folder_id));
        json_object_object_add(
            folder, "folderName",
            json_object_new_string(folders->folders[i].folder_name));
        json_object_object_add(
            folder, "access",
            json_object_new_string(folders->folders[i].access));
        json_object_array_add(folders_array, folder);
    }

    // Add files to response
    for (int i = 0; i < files->count; i++)
    {
        struct json_object *file = json_object_new_object();
        json_object_object_add(file, "fileId",
                               json_object_new_string(files->files[i].file_id));
        json_object_object_add(
            file, "fileName",
            json_object_new_string(files->files[i].file_name));
        json_object_object_add(
            file, "fileSize", json_object_new_int64(files->files[i].file_size));
        json_object_object_add(file, "access",
                               json_object_new_string(files->files[i].access));
        json_object_array_add(files_array, file);
    }

    json_object_object_add(response, "responseCode", json_object_new_int(200));
    json_object_object_add(resp_payload, "folders", folders_array);
    json_object_object_add(resp_payload, "files", files_array);
    json_object_object_add(response, "payload", resp_payload);

    const char *response_str = json_object_to_json_string(response);
    send(client->socket, response_str, strlen(response_str), 0);

    json_object_put(response);
    json_object_put(parsed_json);
    // Free the lists
    free_folder_list(folders);
    free_file_list(files);
}

void handle_folder_create(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_name_obj, *folder_path_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderName", &folder_name_obj);
    json_object_object_get_ex(payload, "folderPath", &folder_path_obj);

    const char *folder_name = json_object_get_string(folder_name_obj);
    char *parent_id = db_get_root_folder_id(client->username);

    const char *folder_path = json_object_get_string(folder_path_obj);
    if (folder_path && strlen(folder_path) > 0)
    {
        char *path_copy = strdup(folder_path);
        char *token = strtok(path_copy, "/");
        char *parent_folder = NULL;

        while (token != NULL)
        {
            parent_folder = token;
            char *new_parent_id = db_get_folder_id(parent_folder, client->username, parent_id);
            free(parent_id);
            parent_id = new_parent_id;

            if (parent_id == NULL)
            {
                send_response(client->socket, 500, "Failed to create folder");
                json_object_put(parsed_json);
                free(path_copy);
                return;
            }
            token = strtok(NULL, "/");
        }
        free(path_copy);
    }

    if (db_create_folder(folder_name, parent_id, client->username))
    {
        send_response(client->socket, 201, "Folder created successfully");
    }
    else
    {
        send_response(client->socket, 500, "Failed to create folder");
    }

    free(parent_id);
    json_object_put(parsed_json);
}

void handle_folder_upload(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_path_obj, *folder_name_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderPath", &folder_path_obj);
    json_object_object_get_ex(payload, "folderName", &folder_name_obj);

    char *folder_path = strdup(json_object_get_string(folder_path_obj));
    const char *folder_name = json_object_get_string(folder_name_obj);

    char *path_copy = strdup(folder_path);
    char *token = strtok(path_copy, "/");
    char *parent_folder = NULL;
    char *parent_id = db_get_root_folder_id(client->username);
    while (token != NULL)
    {
        parent_folder = token;
        parent_id =
            db_get_folder_id(parent_folder, client->username, parent_id);
        if (parent_id == NULL)
        {
            send_response(client->socket, 500, "Failed to upload folder");
            json_object_put(parsed_json);
            free(folder_path);
            free(path_copy);
            return;
        }
        token = strtok(NULL, "/");
    }

    if (db_check_folder_exist(folder_name, client->username, parent_id))
    {
        send_response(client->socket, 500, "Folder already exists");
        json_object_put(parsed_json);
        free(folder_path);
        free(path_copy);
        return;
    }

    if (db_create_folder(folder_name, parent_id, client->username))
    {
        send_response(client->socket, 201, "Folder created successfully");
    }
    else
    {
        send_response(client->socket, 500, "Failed to create folder");
    }
    json_object_put(parsed_json);
    free(folder_path);
    free(path_copy);
}

void handle_folder_download(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_path_obj, *folder_owner_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderPath", &folder_path_obj);
    json_object_object_get_ex(payload, "folderOwner", &folder_owner_obj);

    char *folder_path = strdup(json_object_get_string(folder_path_obj));
    const char *folder_owner = json_object_get_string(folder_owner_obj);
    if (folder_owner == NULL)
    {
        folder_owner = client->username;
    }
    char *path_copy = strdup(folder_path);
    char *token = strtok(path_copy, "/");
    char *parent_folder = NULL;
    char *folder_id = db_get_root_folder_id(folder_owner);
    while (token != NULL)
    {
        parent_folder = token;
        folder_id =
            db_get_folder_id(parent_folder, client->username, folder_id);
        if (folder_id == NULL)
        {
            send_response(client->socket, 500, "Failed to download folder");
            json_object_put(parsed_json);
            free(folder_path);
            free(path_copy);
            return;
        }
        token = strtok(NULL, "/");
    }

    // download_folder_handler(folder_path, folder_owner);
    free(folder_path);
    free(path_copy);
}

void handle_folder_rename(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_path_obj, *new_folder_name_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderPath", &folder_path_obj);
    json_object_object_get_ex(payload, "newFolderName", &new_folder_name_obj);

    const char *folder_path = json_object_get_string(folder_path_obj);
    const char *new_folder_name = json_object_get_string(new_folder_name_obj);

    char *path_copy = strdup(folder_path);
    char *token = strtok(path_copy, "/");
    char *folder_name = NULL;
    char *folder_id = db_get_root_folder_id(client->username);
    while (token != NULL)
    {
        folder_name = token;
        folder_id = db_get_folder_id(folder_name, client->username, folder_id);
        if (folder_id == NULL)
        {
            send_response(client->socket, 500, "Failed to rename folder");
            json_object_put(parsed_json);
            free(path_copy);
            return;
        }
        token = strtok(NULL, "/");
    }

    if (db_rename_folder(folder_id, new_folder_name))
    {
        send_response(client->socket, 200, "Folder renamed successfully");
    }
    else
    {
        send_response(client->socket, 500, "Failed to rename folder");
    }

    json_object_put(parsed_json);
    free(path_copy);
}

void handle_folder_copy(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *from_folder_obj, *to_folder_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fromFolder", &from_folder_obj);
    json_object_object_get_ex(payload, "toFolder", &to_folder_obj);

    const char *from_folder = json_object_get_string(from_folder_obj);
    const char *to_folder = json_object_get_string(to_folder_obj);

    char *path_copy = strdup(from_folder);
    char *from_folder_name = NULL;
    char *from_folder_id = db_get_root_folder_id(client->username);
    while (path_copy != NULL)
    {
        from_folder_name = path_copy;
        from_folder_id = db_get_folder_id(from_folder_name, client->username,
                                          from_folder_id);
        if (from_folder_id == NULL)
        {
            send_response(client->socket, 500, "Failed to copy folder");
            json_object_put(parsed_json);
            free(path_copy);
            return;
        }
        path_copy = strtok(NULL, "/");
    }

    path_copy = strdup(to_folder);
    char *to_folder_name = NULL;
    char *to_folder_id = db_get_root_folder_id(client->username);
    while (path_copy != NULL)
    {
        to_folder_name = path_copy;
        to_folder_id =
            db_get_folder_id(to_folder_name, client->username, to_folder_id);
        if (to_folder_id == NULL)
        {
            send_response(client->socket, 500, "Failed to copy folder");
            json_object_put(parsed_json);
            free(path_copy);
            return;
        }
        path_copy = strtok(NULL, "/");
    }
    if (db_check_folder_exist(to_folder_name, client->username,
                              to_folder_id))
    {
        send_response(client->socket, 500, "Folder already exists");
        json_object_put(parsed_json);
        free(path_copy);
        return;
    }

    if (db_copy_all_content_folder(from_folder_id, to_folder_id))
    {
        send_response(client->socket, 200, "Folder copied successfully");
    }
    else
    {
        send_response(client->socket, 500, "Failed to copy folder");
    }

    json_object_put(parsed_json);
    free(path_copy);
}

void handle_folder_move(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *from_folder_obj, *to_folder_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fromFolder", &from_folder_obj);
    json_object_object_get_ex(payload, "toFolder", &to_folder_obj);

    const char *from_folder = json_object_get_string(from_folder_obj);
    const char *to_folder = json_object_get_string(to_folder_obj);

    char *path_copy = strdup(from_folder);
    char *from_folder_name = NULL;
    char *from_folder_id = db_get_root_folder_id(client->username);
    while (path_copy != NULL)
    {
        from_folder_name = path_copy;
        from_folder_id = db_get_folder_id(from_folder_name, client->username,
                                          from_folder_id);
        if (from_folder_id == NULL)
        {
            send_response(client->socket, 500, "Failed to move folder");
            json_object_put(parsed_json);
            free(path_copy);
            return;
        }
        path_copy = strtok(NULL, "/");
    }

    path_copy = strdup(to_folder);
    char *to_folder_name = NULL;
    char *to_folder_id = db_get_root_folder_id(client->username);
    while (path_copy != NULL)
    {
        to_folder_name = path_copy;
        to_folder_id =
            db_get_folder_id(to_folder_name, client->username, to_folder_id);
        if (to_folder_id == NULL)
        {
            send_response(client->socket, 500, "Failed to move folder");
            json_object_put(parsed_json);
            free(path_copy);
            return;
        }
        path_copy = strtok(NULL, "/");
    }

    if (db_check_folder_exist(to_folder_name, client->username,
                              to_folder_id))
    {
        send_response(client->socket, 500, "Folder already exists");
        json_object_put(parsed_json);
        free(path_copy);
        return;
    }

    if (db_move_folder(from_folder_id, to_folder_id, client->username))
    {
        send_response(client->socket, 200, "Folder moved successfully");
    }
    else
    {
        send_response(client->socket, 500, "Failed to move folder");
    }

    json_object_put(parsed_json);
    free(path_copy);
}

void handle_folder_delete(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_path_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderPath", &folder_path_obj);

    const char *folder_path = json_object_get_string(folder_path_obj);

    char *path_copy = strdup(folder_path);
    char *folder_name = NULL;
    char *folder_id = db_get_root_folder_id(client->username);
    while (path_copy != NULL)
    {
        folder_name = path_copy;
        folder_id = db_get_folder_id(folder_name, client->username, folder_id);
        if (folder_id == NULL)
        {
            send_response(client->socket, 500, "Failed to delete folder");
            json_object_put(parsed_json);
            free(path_copy);
            return;
        }
        path_copy = strtok(NULL, "/");
    }

    if (db_delete_folder(folder_id))
    {
        send_response(client->socket, 200, "Folder deleted successfully");
    }
    else
    {
        send_response(client->socket, 500, "Failed to delete folder");
    }

    json_object_put(parsed_json);
    free(path_copy);
}

void handle_folder_set_access(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_id_obj, *access_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderId", &folder_id_obj);
    json_object_object_get_ex(payload, "access", &access_obj);

    const char *folder_id = json_object_get_string(folder_id_obj);
    const char *access = json_object_get_string(access_obj);

    if (db_set_folder_access(folder_id, access))
    {
        send_response(client->socket, 200,
                      "Folder access updated successfully");
    }
    else
    {
        send_response(client->socket, 500, "Failed to update folder access");
    }

    json_object_put(parsed_json);
}

void handle_folder_get_access(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_id_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderId", &folder_id_obj);

    const char *folder_id = json_object_get_string(folder_id_obj);
    char *access = db_get_folder_access(folder_id);

    if (access)
    {
        struct json_object *response = json_object_new_object();
        struct json_object *resp_payload = json_object_new_object();

        json_object_object_add(response, "responseCode",
                               json_object_new_int(200));
        json_object_object_add(resp_payload, "access",
                               json_object_new_string(access));
        json_object_object_add(response, "payload", resp_payload);

        const char *response_str = json_object_to_json_string(response);
        send(client->socket, response_str, strlen(response_str), 0);

        json_object_put(response);
        free(access);
    }
    else
    {
        send_response(client->socket, 500, "Failed to get folder access");
    }

    json_object_put(parsed_json);
}

void handle_root_folder_create(const char *username)
{
    db_create_root_folder(username);
}

void handle_root_folder_get(client_t *client, const char *buffer)
{
    char *root_folder_id = db_get_root_folder_id(client->username);

    if (root_folder_id)
    {
        struct json_object *response = json_object_new_object();
        struct json_object *resp_payload = json_object_new_object();

        json_object_object_add(response, "responseCode",
                               json_object_new_int(200));
        json_object_object_add(resp_payload, "rootFolderId",
                               json_object_new_string(root_folder_id));
        json_object_object_add(response, "payload", resp_payload);

        const char *response_str = json_object_to_json_string(response);
        send(client->socket, response_str, strlen(response_str), 0);

        json_object_put(response);
        free(root_folder_id);
    }
    else
    {
        send_response(client->socket, 404, "Root folder not found");
    }
}

void handle_folder_search(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *search_term_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderName", &search_term_obj);

    const char *search_term = json_object_get_string(search_term_obj);
    FolderList *folders = db_search_folder(search_term);

    if (folders)
    {
        struct json_object *response = json_object_new_object();
        struct json_object *resp_payload = json_object_new_object();
        struct json_object *folders_array = json_object_new_array();

        for (int i = 0; i < folders->count; i++)
        {
            struct json_object *folder = json_object_new_object();
            json_object_object_add(
                folder, "folderId",
                json_object_new_string(folders->folders[i].folder_id));
            json_object_object_add(
                folder, "folderName",
                json_object_new_string(folders->folders[i].folder_name));
            json_object_object_add(
                folder, "createdBy",
                json_object_new_string(folders->folders[i].created_by));
            json_object_object_add(
                folder, "access",
                json_object_new_string(folders->folders[i].access));
            json_object_array_add(folders_array, folder);
        }

        json_object_object_add(response, "responseCode",
                               json_object_new_int(200));
        json_object_object_add(resp_payload, "folders", folders_array);
        json_object_object_add(response, "payload", resp_payload);

        const char *response_str = json_object_to_json_string(response);
        send(client->socket, response_str, strlen(response_str), 0);

        json_object_put(response);

        // TODO: Add function to free FolderList
        free(folders);
    }
    else
    {
        send_response(client->socket, 500, "Failed to search folders");
    }

    json_object_put(parsed_json);
}

void track_progress(long total_size, long current_size)
{
    double progress = (double)current_size / total_size * 100;
    printf("\rProgress: %.2f%%", progress);
    fflush(stdout);
}

// Helper function to copy folder contents
// Continue with other handlers...