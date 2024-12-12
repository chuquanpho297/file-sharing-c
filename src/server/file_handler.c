#include "file_handler.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>

void handle_upload_file(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *file_name, *file_size, *group_name, *folder_name;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fileName", &file_name);
    json_object_object_get_ex(payload, "fileSize", &file_size);
    json_object_object_get_ex(payload, "groupName", &group_name);
    json_object_object_get_ex(payload, "folderName", &folder_name);

    const char* fname = json_object_get_string(file_name);
    long fsize = json_object_get_int64(file_size);
    const char* gname = json_object_get_string(group_name);
    const char* folder = json_object_get_string(folder_name);

    // Create file entry in database
    if (create_file(fname, fsize, gname, folder)) {
        // Create directory path if it doesn't exist
        char path[1024];
        snprintf(path, sizeof(path), "root/%s/%s", gname, folder);
        mkdir(path, 0777);

        // Create full file path
        char file_path[2048];
        snprintf(file_path, sizeof(file_path), "%s/%s", path, fname);

        // Send success response to client
        send_response(client->socket, 200, "Ready to receive file");

        // Receive and write file
        FILE* fp = fopen(file_path, "wb");
        if (fp != NULL) {
            char buffer[4096];
            long total_received = 0;
            int bytes_received;

            while (total_received < fsize && 
                   (bytes_received = recv(client->socket, buffer, sizeof(buffer), 0)) > 0) {
                fwrite(buffer, 1, bytes_received, fp);
                total_received += bytes_received;
            }

            fclose(fp);
            send_response(client->socket, 200, "File uploaded successfully");
        } else {
            send_response(client->socket, 500, "Failed to create file");
        }
    } else {
        send_response(client->socket, 409, "File already exists or invalid location");
    }

    json_object_put(parsed_json);
}

void handle_download_file(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *file_name, *group_name, *folder_name;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fileName", &file_name);
    json_object_object_get_ex(payload, "groupName", &group_name);
    json_object_object_get_ex(payload, "folderName", &folder_name);

    const char* fname = json_object_get_string(file_name);
    const char* gname = json_object_get_string(group_name);
    const char* folder = json_object_get_string(folder_name);

    char file_path[2048];
    snprintf(file_path, sizeof(file_path), "root/%s/%s/%s", gname, folder, fname);

    struct stat st;
    if (stat(file_path, &st) == 0) {
        // File exists, send file info response
        struct json_object *response = json_object_new_object();
        struct json_object *resp_payload = json_object_new_object();
        
        json_object_object_add(response, "responseCode", json_object_new_int(200));
        json_object_object_add(resp_payload, "fileSize", json_object_new_int64(st.st_size));
        json_object_object_add(response, "payload", resp_payload);

        const char* response_str = json_object_to_json_string(response);
        send(client->socket, response_str, strlen(response_str), 0);
        json_object_put(response);

        // Send file content
        FILE* fp = fopen(file_path, "rb");
        if (fp != NULL) {
            char buffer[4096];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
                send(client->socket, buffer, bytes_read, 0);
            }
            fclose(fp);
        }
    } else {
        send_response(client->socket, 404, "File not found");
    }

    json_object_put(parsed_json);
}

void handle_file_rename(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *file_name, *new_name, *group_name, *folder_name;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fileName", &file_name);
    json_object_object_get_ex(payload, "newFileName", &new_name);
    json_object_object_get_ex(payload, "groupName", &group_name);
    json_object_object_get_ex(payload, "folderName", &folder_name);

    const char* fname = json_object_get_string(file_name);
    const char* new_fname = json_object_get_string(new_name);
    const char* gname = json_object_get_string(group_name);
    const char* folder = json_object_get_string(folder_name);

    if (rename_file(fname, new_fname, gname, folder)) {
        // Rename physical file
        char old_path[2048], new_path[2048];
        snprintf(old_path, sizeof(old_path), "root/%s/%s/%s", gname, folder, fname);
        snprintf(new_path, sizeof(new_path), "root/%s/%s/%s", gname, folder, new_fname);
        
        if (rename(old_path, new_path) == 0) {
            send_response(client->socket, 200, "File renamed successfully");
        } else {
            send_response(client->socket, 500, "Failed to rename file");
        }
    } else {
        send_response(client->socket, 404, "File not found or invalid operation");
    }

    json_object_put(parsed_json);
}

void handle_file_copy(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload;
    json_object_object_get_ex(parsed_json, "payload", &payload);

    const char* fname = json_object_get_string(json_object_object_get(payload, "fileName"));
    const char* from_group = json_object_get_string(json_object_object_get(payload, "fromGroup"));
    const char* to_group = json_object_get_string(json_object_object_get(payload, "toGroup"));
    const char* from_folder = json_object_get_string(json_object_object_get(payload, "fromFolder"));
    const char* to_folder = json_object_get_string(json_object_object_get(payload, "toFolder"));

    // Get file size from source file
    char src_path[2048];
    snprintf(src_path, sizeof(src_path), "root/%s/%s/%s", from_group, from_folder, fname);
    struct stat st;
    if (stat(src_path, &st) == 0) {
        if (copy_file(fname, st.st_size, from_group, to_group, from_folder, to_folder)) {
            char dst_path[2048];
            snprintf(dst_path, sizeof(dst_path), "root/%s/%s", to_group, to_folder);
            mkdir(dst_path, 0777);
            snprintf(dst_path, sizeof(dst_path), "root/%s/%s/%s", to_group, to_folder, fname);
            
            FILE *src = fopen(src_path, "rb");
            FILE *dst = fopen(dst_path, "wb");
            if (src && dst) {
                char buf[4096];
                size_t n;
                while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
                    fwrite(buf, 1, n, dst);
                }
                fclose(src);
                fclose(dst);
                send_response(client->socket, 200, "File copied successfully");
            } else {
                send_response(client->socket, 500, "Failed to copy file");
            }
        } else {
            send_response(client->socket, 403, "Operation not permitted");
        }
    } else {
        send_response(client->socket, 404, "Source file not found");
    }
    json_object_put(parsed_json);
}

void handle_file_move(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload;
    json_object_object_get_ex(parsed_json, "payload", &payload);

    const char* fname = json_object_get_string(json_object_object_get(payload, "fileName"));
    const char* from_group = json_object_get_string(json_object_object_get(payload, "fromGroup"));
    const char* to_group = json_object_get_string(json_object_object_get(payload, "toGroup"));
    const char* from_folder = json_object_get_string(json_object_object_get(payload, "fromFolder"));
    const char* to_folder = json_object_get_string(json_object_object_get(payload, "toFolder"));

    char src_path[2048], dst_path[2048];
    snprintf(src_path, sizeof(src_path), "root/%s/%s/%s", from_group, from_folder, fname);
    
    struct stat st;
    if (stat(src_path, &st) == 0) {
        if (move_file(fname, st.st_size, from_group, to_group, from_folder, to_folder)) {
            snprintf(dst_path, sizeof(dst_path), "root/%s/%s", to_group, to_folder);
            mkdir(dst_path, 0777);
            snprintf(dst_path, sizeof(dst_path), "root/%s/%s/%s", to_group, to_folder, fname);
            
            if (rename(src_path, dst_path) == 0) {
                send_response(client->socket, 200, "File moved successfully");
            } else {
                send_response(client->socket, 500, "Failed to move file");
            }
        } else {
            send_response(client->socket, 403, "Operation not permitted");
        }
    } else {
        send_response(client->socket, 404, "Source file not found");
    }
    json_object_put(parsed_json);
}

void handle_file_delete(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload;
    json_object_object_get_ex(parsed_json, "payload", &payload);

    const char* fname = json_object_get_string(json_object_object_get(payload, "fileName"));
    const char* group_name = json_object_get_string(json_object_object_get(payload, "groupName"));
    const char* folder_name = json_object_get_string(json_object_object_get(payload, "folderName"));

    if (delete_file(fname, group_name, folder_name)) {
        char file_path[2048];
        snprintf(file_path, sizeof(file_path), "root/%s/%s/%s", group_name, folder_name, fname);
        if (remove(file_path) == 0) {
            send_response(client->socket, 200, "File deleted successfully");
        } else {
            send_response(client->socket, 500, "Failed to delete file");
        }
    } else {
        send_response(client->socket, 404, "File not found or operation not permitted");
    }
    json_object_put(parsed_json);
}

 