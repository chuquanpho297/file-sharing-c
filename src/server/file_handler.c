#include "file_handler.h"
#include "controllers/file_controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void send_response(int socket, int code, const char* message) {
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "{\"responseCode\": %d, \"message\": \"%s\"}", code, message);
    send(socket, response, strlen(response), 0);
}

void handle_create_file(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *file_name_obj, *file_size_obj, *folder_id_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fileName", &file_name_obj);
    json_object_object_get_ex(payload, "fileSize", &file_size_obj);
    json_object_object_get_ex(payload, "folderId", &folder_id_obj);
    
    const char* file_name = json_object_get_string(file_name_obj);
    long file_size = json_object_get_int64(file_size_obj);
    const char* folder_id = json_object_get_string(folder_id_obj);

    if (create_file(file_name, file_size, folder_id)) {
        send_response(client->socket, 201, "File created successfully");
    } else {
        send_response(client->socket, 500, "Failed to create file");
    }

    json_object_put(parsed_json);
}

void handle_copy_file(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *file_id_obj, *folder_id_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fileId", &file_id_obj);
    json_object_object_get_ex(payload, "toFolderId", &folder_id_obj);
    
    const char* file_id = json_object_get_string(file_id_obj);
    const char* to_folder_id = json_object_get_string(folder_id_obj);

    if (copy_file(file_id, to_folder_id)) {
        send_response(client->socket, 200, "File copied successfully");
    } else {
        send_response(client->socket, 500, "Failed to copy file");
    }

    json_object_put(parsed_json);
}

void handle_move_file(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *file_id_obj, *folder_id_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fileId", &file_id_obj);
    json_object_object_get_ex(payload, "toFolderId", &folder_id_obj);
    
    const char* file_id = json_object_get_string(file_id_obj);
    const char* to_folder_id = json_object_get_string(folder_id_obj);

    if (move_file(file_id, to_folder_id)) {
        send_response(client->socket, 200, "File moved successfully");
    } else {
        send_response(client->socket, 500, "Failed to move file");
    }

    json_object_put(parsed_json);
}

void handle_rename_file(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *file_id_obj, *new_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fileId", &file_id_obj);
    json_object_object_get_ex(payload, "newName", &new_name_obj);
    
    const char* file_id = json_object_get_string(file_id_obj);
    const char* new_name = json_object_get_string(new_name_obj);

    if (rename_file(file_id, new_name)) {
        send_response(client->socket, 200, "File renamed successfully");
    } else {
        send_response(client->socket, 500, "Failed to rename file");
    }

    json_object_put(parsed_json);
}

void handle_delete_file(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *file_id_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fileId", &file_id_obj);
    
    const char* file_id = json_object_get_string(file_id_obj);

    if (delete_file(file_id)) {
        send_response(client->socket, 200, "File deleted successfully");
    } else {
        send_response(client->socket, 500, "Failed to delete file");
    }

    json_object_put(parsed_json);
}

void handle_file_set_access(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *file_id_obj, *access_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fileId", &file_id_obj);
    json_object_object_get_ex(payload, "access", &access_obj);
    
    const char* file_id = json_object_get_string(file_id_obj);
    const char* access = json_object_get_string(access_obj);

    if (set_file_access(file_id, access)) {
        send_response(client->socket, 200, "File access updated successfully");
    } else {
        send_response(client->socket, 500, "Failed to update file access");
    }

    json_object_put(parsed_json);
}

void handle_file_get_access(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *file_id_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fileId", &file_id_obj);
    
    const char* file_id = json_object_get_string(file_id_obj);
    char* access = get_file_access(file_id);

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
        send_response(client->socket, 500, "Failed to get file access");
    }

    json_object_put(parsed_json);
}

void handle_search_files(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *search_term_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "searchTerm", &search_term_obj);
    
    const char* search_term = json_object_get_string(search_term_obj);
    FileList* files = search_files(search_term);

    if (files) {
        struct json_object *response = json_object_new_object();
        struct json_object *resp_payload = json_object_new_object();
        struct json_object *files_array = json_object_new_array();

        for (int i = 0; i < files->count; i++) {
            struct json_object *file = json_object_new_object();
            json_object_object_add(file, "fileId", json_object_new_string(files->files[i].file_id));
            json_object_object_add(file, "fileName", json_object_new_string(files->files[i].file_name));
            json_object_object_add(file, "fileSize", json_object_new_int64(files->files[i].file_size));
            json_object_object_add(file, "access", json_object_new_string(files->files[i].access));
            json_object_array_add(files_array, file);
        }

        json_object_object_add(response, "responseCode", json_object_new_int(200));
        json_object_object_add(resp_payload, "files", files_array);
        json_object_object_add(response, "payload", resp_payload);

        const char* response_str = json_object_to_json_string(response);
        send(client->socket, response_str, strlen(response_str), 0);

        json_object_put(response);
        // TODO: Add function to free FileList
    } else {
        send_response(client->socket, 500, "Failed to search files");
    }

    json_object_put(parsed_json);
}
