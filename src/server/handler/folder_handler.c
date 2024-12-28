#include "folder_handler.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../utils/config.h"
#include "../../utils/helper.h"
#include "../../utils/structs.h"
#include "../db/db_access.h"
#include "../system/system_access.h"
#include "./file_handler.h"

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
            char *new_parent_id =
                db_get_folder_id(parent_folder, client->username, parent_id);
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

    char path[4096];
    if (folder_path && strlen(folder_path) > 0)
        snprintf(path, sizeof(path), "%s/%s/%s/%s", ROOT_FOLDER,
                 client->username, folder_path, folder_name);
    else
        snprintf(path, sizeof(path), "%s/%s/%s", ROOT_FOLDER, client->username,
                 folder_name);

    struct stat st = {0};
    if (stat(path, &st) == 0)
    {
        send_response(client->socket, 409, "Folder already exists");
    }
    else
    {
        if (mkdir(path, 0777) == 0)
        {
            if (db_create_folder(folder_name, parent_id, client->username))
            {
                send_response(client->socket, 201,
                              "Folder created successfully");
            }
            else
            {
                rmdir(path);
                send_response(client->socket, 500, "Failed to create folder");
            }
        }
        else
        {
            send_response(client->socket, 501, "Failed to create folder");
        }
    }

    free(parent_id);
    json_object_put(parsed_json);
}

void handle_folder_upload(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_path_obj, *folder_name_obj,
        *file_number_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderPath", &folder_path_obj);
    json_object_object_get_ex(payload, "folderName", &folder_name_obj);
    json_object_object_get_ex(payload, "fileCount", &file_number_obj);

    char *folder_path = strdup(json_object_get_string(folder_path_obj));
    const char *folder_name = json_object_get_string(folder_name_obj);
    int file_number = json_object_get_int(file_number_obj);

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
            send_response(client->socket, 404,
                          strcat("Folder not found: ", parent_folder));
            json_object_put(parsed_json);
            free(folder_path);
            free(path_copy);
            return;
        }
        token = strtok(NULL, "/");
    }

    // if folder name existed, add version number
    int version = 1;
    char *new_folder_name = strdup(folder_name);

    while (db_check_folder_exist(new_folder_name, client->username, parent_id))
    {
        snprintf(new_folder_name, MAX_FOLDER_NAME, "%s(%d)", folder_name,
                 version);
        version++;
    }

    folder_name = new_folder_name;

    char parent_upload_folder_path[MAX_PATH_LENGTH];

    if (strlen(folder_path) > 0)
        snprintf(parent_upload_folder_path, sizeof(parent_upload_folder_path),
                 "%s/%s/%s", ROOT_FOLDER, client->username, folder_path);
    else
        snprintf(parent_upload_folder_path, sizeof(parent_upload_folder_path),
                 "%s/%s", ROOT_FOLDER, client->username);

    // Calculate required space
    size_t required_len = strlen(parent_upload_folder_path) +
                          strlen(folder_name) +
                          2;  // +2 for '/' and null terminator
    if (required_len > MAX_PATH_LENGTH)
    {
        send_response(client->socket, 400, "Path too long");
        json_object_put(parsed_json);
        free(folder_path);
        free(path_copy);
        return;
    }

    char *upload_folder_path[MAX_PATH_LENGTH];
    snprintf(upload_folder_path, MAX_PATH_LENGTH, "%s/%s",
             parent_upload_folder_path, folder_name);

    // Create empty upload folder
    if (!create_directories(upload_folder_path))
    {
        send_response(client->socket, 500, "Failed in uploading folder");
        json_object_put(parsed_json);
        free(folder_path);
        free(path_copy);
        return;
    }

    if (!db_create_folder(folder_name, parent_id, client->username))
    {
        send_response(client->socket, 500, "Failed in uploading folder");
        json_object_put(parsed_json);
        free(folder_path);
        free(path_copy);
        return;
    }

    struct json_object *readyResponse = json_object_new_object();
    struct json_object *readyResponsePayload = json_object_new_object();
    json_object_object_add(readyResponse, "responseCode",
                           json_object_new_int(200));
    json_object_object_add(readyResponsePayload, "message",
                           json_object_new_string("Ready to upload folder"));
    json_object_object_add(readyResponsePayload, "validFolderName",
                           json_object_new_string(folder_name));
    json_object_object_add(readyResponse, "payload", readyResponsePayload);

    const char *readyResponseStr = json_object_to_json_string(readyResponse);
    send(client->socket, readyResponseStr, strlen(readyResponseStr), 0);

    // Receive files
    int file_count = 0;

    while (file_count < file_number)
    {
        char buffer[BUFFER_SIZE];
        recv(client->socket, buffer, BUFFER_SIZE, 0);

        if (strcmp(buffer, CLIENT_ERROR) == 0)
        {
            printf("Failed to receive file: %s\n", file_count);
            file_count++;
            continue;
        }

        struct json_object *file_info = json_tokener_parse(buffer);
        struct json_object *file_path_obj, *file_size_obj;
        json_object_object_get_ex(file_info, "filePath", &file_path_obj);
        json_object_object_get_ex(file_info, "fileSize", &file_size_obj);
        char *short_file_path = json_object_get_string(file_path_obj);
        long file_size = json_object_get_int64(file_size_obj);

        char *file_path[MAX_PATH_LENGTH];
        snprintf(file_path, sizeof(file_path) + 1, "%s/%s/%s", ROOT_FOLDER,
                 client->username, short_file_path);

        // Create directories if they do not exist
        char *last_slash = strrchr(file_path, '/');
        if (last_slash)
        {
            *last_slash = '\0';
            if (!create_directories(file_path))
            {
                send_response(client->socket, 500, "Failed to create folder");
                json_object_put(file_info);
                free(folder_path);
                free(path_copy);
                return;
            }
            *last_slash = '/';
        }

        send_response(client->socket, 200, "Ready to receive file");

        FILE *file = fopen(file_path, "wb");

        printf("Receiving file: %s\n", file_path);
        receive_write_file(client->socket, file_size, file);

        // Update db
        char *short_folder_path = get_folder_path(short_file_path);
        char *sub_parent_id =
            db_get_folder_id(folder_name, client->username, parent_id);
        char *sub_folder = strtok(short_folder_path, "/");
        sub_folder = strtok(NULL, "/");  // Skip the upload folder name

        // create sub folders
        while (sub_folder != NULL)
        {
            if (!db_check_folder_exist(sub_folder, client->username,
                                       sub_parent_id))
            {
                db_create_folder(sub_folder, sub_parent_id, client->username);
            }
            sub_parent_id =
                db_get_folder_id(sub_folder, client->username, sub_parent_id);
            sub_folder = strtok(NULL, "/");
        }

        db_create_file(get_filename(file_path), file_size, sub_parent_id,
                       client->username);

        file_count++;
        printf("File count: %d/%d\n", file_count, file_number);

        json_object_put(file_info);
    }

    send_response(client->socket, 200, "Folder uploaded successfully");

    json_object_put(readyResponse);
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

    if (folder_owner == "")
    {
        folder_owner = client->username;
    }

    char *path_copy = strdup(folder_path);
    char *token = strtok(path_copy, "/");
    char *folder_name = NULL;
    char *parent_id = db_get_root_folder_id(folder_owner);

    while (token != NULL)
    {
        folder_name = token;
        parent_id = db_get_folder_id(folder_name, folder_owner, parent_id);
        if (parent_id == NULL)
        {
            send_response(client->socket, 500, "Failed to download folder");
            json_object_put(parsed_json);
            free(folder_path);
            free(path_copy);
            return;
        }
        token = strtok(NULL, "/");
    }

    const exact_folder_path[MAX_PATH_LENGTH];

    if (folder_path == "")
    {
        folder_name = client->username;
        snprintf(exact_folder_path, MAX_PATH_LENGTH, "%s/%s", ROOT_FOLDER,
                 folder_owner);
    }
    else
        snprintf(exact_folder_path, MAX_PATH_LENGTH, "%s/%s/%s", ROOT_FOLDER,
                 folder_owner, folder_path);

    printf("Compressing folder...\n");
    // Check if temp_folder_path exists, if not, create it
    const char *temp_folder_path[MAX_PATH_LENGTH];
    snprintf(temp_folder_path, MAX_PATH_LENGTH, "%s/%s", TEMP_FOLDER,
             client->username);

    create_directories(temp_folder_path);
    // Zip the folder
    const char temp_zip_folder_path[MAX_PATH_LENGTH];
    const char temp_zip_folder_name[MAX_FOLDER_NAME];

    snprintf(temp_zip_folder_name, MAX_FOLDER_NAME, "%s.zip", folder_name);
    snprintf(temp_zip_folder_path, MAX_PATH_LENGTH, "%s/%s", temp_folder_path,
             temp_zip_folder_name);

    printf("Compressing folder to %s\n", temp_zip_folder_path);
    printf("Folder path: %s\n", exact_folder_path);
    if (!compress_folder(exact_folder_path, temp_zip_folder_path))
    {
        send_response(client->socket, 500, "Failed to compress folder");
        json_object_put(parsed_json);
        free(folder_path);
        free(path_copy);
        return;
    }

    // Send the zip file
    FILE *fp = fopen(temp_zip_folder_path, "rb");

    if (fp != NULL)
    {
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        struct json_object *response = json_object_new_object();
        struct json_object *response_payload = json_object_new_object();
        json_object_object_add(response, "responseCode",
                               json_object_new_int(200));
        json_object_object_add(response_payload, "fileSize",
                               json_object_new_int64(file_size));
        json_object_object_add(response_payload, "zipFoldername",
                               json_object_new_string(temp_zip_folder_name));
        json_object_object_add(
            response_payload, "message",
            json_object_new_string("Ready to download folder"));
        json_object_object_add(response, "payload", response_payload);
        char *response_str = json_object_to_json_string(response);
        send(client->socket, response_str, strlen(response_str), 0);

        json_object_put(response);

        char buffer[BUFFER_SIZE];
        int recv_len = recv(client->socket, buffer, BUFFER_SIZE, 0);
        buffer[recv_len] = '\0';

        if (strcmp(buffer, "OK") != 0)
        {
            send_response(client->socket, 500, "Failed to download folder");
            json_object_put(parsed_json);
            free(folder_path);
            free(path_copy);
            return;
        }

        read_send_file(client->socket, file_size, fp);
        remove(temp_zip_folder_path);
    }
    else
    {
        send_response(client->socket, 500, "Failed to download folder");
    }

    json_object_put(parsed_json);
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

    // Construct the new folder path
    const char *parent_folder_path = get_folder_path(folder_path);
    char new_folder_path[MAX_PATH_LENGTH];
    if (parent_folder_path == NULL)
    {
        snprintf(new_folder_path, MAX_PATH_LENGTH, "%s/%s/%s", ROOT_FOLDER,
                 client->username, new_folder_name);
    }
    else
        snprintf(new_folder_path, MAX_PATH_LENGTH, "%s/%s/%s/%s", ROOT_FOLDER,
                 client->username, parent_folder_path, new_folder_name);

    char exact_folder_path[MAX_PATH_LENGTH];
    snprintf(exact_folder_path, MAX_PATH_LENGTH, "%s/%s/%s", ROOT_FOLDER,
             client->username, folder_path);

    printf("Renaming folder %s to %s\n", exact_folder_path, new_folder_path);

    if (rename(exact_folder_path, new_folder_path))
    {
        send_response(client->socket, 500, "Failed to rename folder");
        json_object_put(parsed_json);
        free(path_copy);
        return;
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

    // Check if to_folder contains from_folder
    if (strstr(to_folder, from_folder) != NULL)
    {
        send_response(client->socket, 500,
                      "Cannot copy a folder into itself or its subfolder");
        json_object_put(parsed_json);
        return;
    }

    char *path_copy = strdup(from_folder);
    char *from_folder_id = db_get_root_folder_id(client->username);
    char *from_folder_name = strtok(path_copy, "/");

    while (from_folder_name != NULL)
    {
        from_folder_id = db_get_folder_id(from_folder_name, client->username,
                                          from_folder_id);
        if (from_folder_id == NULL)
        {
            send_response(client->socket, 500, "Failed to copy folder");
            json_object_put(parsed_json);
            free(from_folder_name);
            return;
        }
        from_folder_name = strtok(NULL, "/");
    }

    path_copy = strdup(to_folder);
    char *to_folder_name = strtok(path_copy, "/");
    char *to_folder_id = db_get_root_folder_id(client->username);
    while (to_folder_name != NULL)
    {
        to_folder_id =
            db_get_folder_id(to_folder_name, client->username, to_folder_id);
        if (to_folder_id == NULL)
        {
            send_response(client->socket, 500, "Failed to copy folder");
            json_object_put(parsed_json);
            free(path_copy);
            return;
        }
        to_folder_name = strtok(NULL, "/");
    }

    // check if from_folder_name is same name with subfolder of to_folder_name
    if (db_check_folder_exist(get_folder_name(from_folder), client->username,
                              to_folder_id))
    {
        send_response(client->socket, 500, "Folder already exists");
        json_object_put(parsed_json);
        free(path_copy);
        return;
    }

    // copy folder in the system
    char from_folder_path[MAX_PATH_LENGTH];
    char to_folder_path[MAX_PATH_LENGTH];

    snprintf(from_folder_path, MAX_PATH_LENGTH, "%s/%s/%s", ROOT_FOLDER,
             client->username, from_folder);
    snprintf(to_folder_path, MAX_PATH_LENGTH, "%s/%s/%s", ROOT_FOLDER,
             client->username, to_folder);

    if (!copy_folder(from_folder_path, to_folder_path))
    {
        send_response(client->socket, 500, "Failed to copy folder");
        json_object_put(parsed_json);
        free(path_copy);
        return;
    }

    if (db_copy_folder(from_folder_id, to_folder_id))
    {
        printf("Folder copied successfully");
        send_response(client->socket, 200, "Folder copied successfully");
    }
    else
    {
        send_response(client->socket, 500, "Failed to copy folder");
    }

    json_object_put(parsed_json);
    free(path_copy);
}
// TODO: add function to handle folder move
// TODO: add log upload download

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

    if (db_check_folder_exist(to_folder_name, client->username, to_folder_id))
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

    // Construct the new folder path
    const exact_path[MAX_PATH_LENGTH];
    if (path_copy == "")
    {
        snprintf(exact_path, MAX_PATH_LENGTH, "%s/%s", ROOT_FOLDER,
                 client->username);
    }
    else
        snprintf(exact_path, MAX_PATH_LENGTH, "%s/%s/%s", ROOT_FOLDER,
                 client->username, path_copy);

    printf("Deleting folder %s\n", exact_path);

    if (remove_directory(exact_path) != 0)
    {
        printf("Error code: %d\n", errno);
        send_response(client->socket, 500, "Failed to delete folder");
        json_object_put(parsed_json);
        free(path_copy);
        return;
    }

    char *token = strtok(path_copy, "/");

    while (token != NULL)
    {
        folder_name = token;
        folder_id = db_get_folder_id(folder_name, client->username, folder_id);
        if (folder_id == NULL)
        {
            send_response(client->socket, 500, "Failed to delete folder");
            json_object_put(parsed_json);
            free(token);
            return;
        }
        token = strtok(NULL, "/");
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

    if (db_set_folder_access(folder_id, access, client->username))
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
            json_object_object_add(
                folder, "folderPath",
                json_object_new_string(folders->folders[i].folder_path));
            json_object_array_add(folders_array, folder);
        }

        json_object_object_add(response, "responseCode",
                               json_object_new_int(200));
        json_object_object_add(resp_payload, "folders", folders_array);
        json_object_object_add(response, "payload", resp_payload);

        const char *response_str = json_object_to_json_string(response);
        send(client->socket, response_str, strlen(response_str), 0);

        json_object_put(response);

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