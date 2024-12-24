#include "folder_handler.h"
#include "controllers/folder_controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

static void send_response(int socket, int code, const char* message) {
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "{\"responseCode\": %d, \"message\": \"%s\"}", code, message);
    send(socket, response, strlen(response), 0);
}

void handle_folder_content(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_id_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderId", &folder_id_obj);
    
    const char* folder_id = json_object_get_string(folder_id_obj);

    FolderList* folders = get_all_folders_in_folder(folder_id);
    FileList* files = get_all_files_in_folder(folder_id);

    if (!folders || !files) {
        send_response(client->socket, 500, "Failed to get folder contents");
        json_object_put(parsed_json);
        return;
    }

    struct json_object *response = json_object_new_object();
    struct json_object *resp_payload = json_object_new_object();
    struct json_object *folders_array = json_object_new_array();
    struct json_object *files_array = json_object_new_array();

    // Add folders to response
    for (int i = 0; i < folders->count; i++) {
        struct json_object *folder = json_object_new_object();
        json_object_object_add(folder, "folderId", json_object_new_string(folders->folders[i].folder_id));
        json_object_object_add(folder, "folderName", json_object_new_string(folders->folders[i].folder_name));
        json_object_object_add(folder, "access", json_object_new_string(folders->folders[i].access));
        json_object_array_add(folders_array, folder);
    }

    // Add files to response
    for (int i = 0; i < files->count; i++) {
        struct json_object *file = json_object_new_object();
        json_object_object_add(file, "fileId", json_object_new_string(files->files[i].file_id));
        json_object_object_add(file, "fileName", json_object_new_string(files->files[i].file_name));
        json_object_object_add(file, "fileSize", json_object_new_int64(files->files[i].file_size));
        json_object_object_add(file, "access", json_object_new_string(files->files[i].access));
        json_object_array_add(files_array, file);
    }

    json_object_object_add(response, "responseCode", json_object_new_int(200));
    json_object_object_add(resp_payload, "folders", folders_array);
    json_object_object_add(resp_payload, "files", files_array);
    json_object_object_add(response, "payload", resp_payload);

    const char* response_str = json_object_to_json_string(response);
    send(client->socket, response_str, strlen(response_str), 0);

    json_object_put(response);
    json_object_put(parsed_json);
    // Free the lists
    // TODO: Add functions to free these lists
}

void handle_create_folder(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_name_obj, *parent_id_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderName", &folder_name_obj);
    json_object_object_get_ex(payload, "parentId", &parent_id_obj);
    
    const char* folder_name = json_object_get_string(folder_name_obj);
    const char* parent_id = json_object_get_string(parent_id_obj);

    if (create_folder(folder_name, parent_id)) {
        send_response(client->socket, 201, "Folder created successfully");
    } else {
        send_response(client->socket, 500, "Failed to create folder");
    }

    json_object_put(parsed_json);
}

void handle_folder_rename(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_id_obj, *new_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderId", &folder_id_obj);
    json_object_object_get_ex(payload, "newName", &new_name_obj);
    
    const char* folder_id = json_object_get_string(folder_id_obj);
    const char* new_name = json_object_get_string(new_name_obj);

    if (rename_folder(folder_id, new_name)) {
        send_response(client->socket, 200, "Folder renamed successfully");
    } else {
        send_response(client->socket, 500, "Failed to rename folder");
    }

    json_object_put(parsed_json);
}

void handle_folder_copy(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_id_obj, *parent_id_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderId", &folder_id_obj);
    json_object_object_get_ex(payload, "parentId", &parent_id_obj);
    
    const char* folder_id = json_object_get_string(folder_id_obj);
    const char* parent_id = json_object_get_string(parent_id_obj);

    if (copy_folder(folder_id, parent_id, client->username)) {
        send_response(client->socket, 200, "Folder copied successfully");
    } else {
        send_response(client->socket, 500, "Failed to copy folder");
    }

    json_object_put(parsed_json);
}

void handle_folder_move(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_id_obj, *parent_id_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderId", &folder_id_obj);
    json_object_object_get_ex(payload, "parentId", &parent_id_obj);
    
    const char* folder_id = json_object_get_string(folder_id_obj);
    const char* parent_id = json_object_get_string(parent_id_obj);

    if (move_folder(folder_id, parent_id, client->username)) {
        send_response(client->socket, 200, "Folder moved successfully");
    } else {
        send_response(client->socket, 500, "Failed to move folder");
    }

    json_object_put(parsed_json);
}

void handle_folder_delete(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_id_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderId", &folder_id_obj);
    
    const char* folder_id = json_object_get_string(folder_id_obj);

    if (delete_folder(folder_id)) {
        send_response(client->socket, 200, "Folder deleted successfully");
    } else {
        send_response(client->socket, 500, "Failed to delete folder");
    }

    json_object_put(parsed_json);
}

void handle_folder_set_access(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_id_obj, *access_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderId", &folder_id_obj);
    json_object_object_get_ex(payload, "access", &access_obj);
    
    const char* folder_id = json_object_get_string(folder_id_obj);
    const char* access = json_object_get_string(access_obj);

    if (set_folder_access(folder_id, access)) {
        send_response(client->socket, 200, "Folder access updated successfully");
    } else {
        send_response(client->socket, 500, "Failed to update folder access");
    }

    json_object_put(parsed_json);
}

void handle_folder_get_access(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_id_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderId", &folder_id_obj);
    
    const char* folder_id = json_object_get_string(folder_id_obj);
    char* access = get_folder_access(folder_id);

    if (access) {
        struct json_object *response = json_object_new_object();
        struct json_object *resp_payload = json_object_new_object();
        
        json_object_object_add(response, "responseCode", json_object_new_int(200));
        json_object_object_add(resp_payload, "access", json_object_new_string(access));
        json_object_object_add(response, "payload", resp_payload);

        const char* response_str = json_object_to_json_string(response);
        send(client->socket, response_str, strlen(response_str), 0);

        json_object_put(response);
        free(access);
    } else {
        send_response(client->socket, 500, "Failed to get folder access");
    }

    json_object_put(parsed_json);
}

void handle_create_root_folder(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *folder_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "folderName", &folder_name_obj);
    
    const char* folder_name = json_object_get_string(folder_name_obj);

    if (create_root_folder(folder_name, client->username)) {
        send_response(client->socket, 201, "Root folder created successfully");
    } else {
        send_response(client->socket, 500, "Failed to create root folder");
    }

    json_object_put(parsed_json);
}

void handle_get_root_folder(client_t* client, const char* buffer) {
    char* root_folder_id = get_root_folder_id(client->username);
    
    if (root_folder_id) {
        struct json_object *response = json_object_new_object();
        struct json_object *resp_payload = json_object_new_object();
        
        json_object_object_add(response, "responseCode", json_object_new_int(200));
        json_object_object_add(resp_payload, "rootFolderId", json_object_new_string(root_folder_id));
        json_object_object_add(response, "payload", resp_payload);

        const char* response_str = json_object_to_json_string(response);
        send(client->socket, response_str, strlen(response_str), 0);

        json_object_put(response);
        free(root_folder_id);
    } else {
        send_response(client->socket, 404, "Root folder not found");
    }
}

void handle_search_folders(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *search_term_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "searchTerm", &search_term_obj);
    
    const char* search_term = json_object_get_string(search_term_obj);
    FolderList* folders = search_folders(search_term);

    if (folders) {
        struct json_object *response = json_object_new_object();
        struct json_object *resp_payload = json_object_new_object();
        struct json_object *folders_array = json_object_new_array();

        for (int i = 0; i < folders->count; i++) {
            struct json_object *folder = json_object_new_object();
            json_object_object_add(folder, "folderId", json_object_new_string(folders->folders[i].folder_id));
            json_object_object_add(folder, "folderName", json_object_new_string(folders->folders[i].folder_name));
            json_object_object_add(folder, "createdBy", json_object_new_string(folders->folders[i].created_by));
            json_object_object_add(folder, "access", json_object_new_string(folders->folders[i].access));
            json_object_array_add(folders_array, folder);
        }

        json_object_object_add(response, "responseCode", json_object_new_int(200));
        json_object_object_add(resp_payload, "folders", folders_array);
        json_object_object_add(response, "payload", resp_payload);

        const char* response_str = json_object_to_json_string(response);
        send(client->socket, response_str, strlen(response_str), 0);

        json_object_put(response);
        // TODO: Add function to free FolderList
    } else {
        send_response(client->socket, 500, "Failed to search folders");
    }

    json_object_put(parsed_json);
}


static void track_progress(long total_size, long current_size) {
    double progress = (double)current_size / total_size * 100;
    printf("\rProgress: %.2f%%", progress);
    fflush(stdout);
}

// Helper function to clear line for progress display
static void clear_line(void) {
    printf("\r");
    fflush(stdout);
}

// Helper function to copy folder contents
// Continue with other handlers... 