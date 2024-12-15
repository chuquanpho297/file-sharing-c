#include "group_handler.h"
#include "db/db_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

// Helper function declarations
static void track_progress(long total_size, long current_size);
static void clear_line(void);
static void copy_folder(const char* src_path, const char* dst_path);
static void delete_folder(const char* path);

// All the existing handler implementations remain the same, 
// but remove duplicates and keep them in this order:

// Group management handlers
void handle_create_group(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);
    
    // Create group directory
    char path[1024];
    snprintf(path, sizeof(path), "root/%s", group_name);
    
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        // Directory doesn't exist, try to create it
        if (mkdir(path, 0777) == 0) {
            if (db_create_group(client->username, group_name)) {
                send_response(client->socket, 201, "Group created successfully");
            } else {
                rmdir(path); // Rollback directory creation
                send_response(client->socket, 501, "Failed to create group in database");
            }
        } else {
            send_response(client->socket, 501, "Failed to create group directory");
        }
    } else {
        send_response(client->socket, 409, "Group already exists");
    }
    
    json_object_put(parsed_json);
}

void handle_list_all_groups(client_t* client, const char* buffer) {
    GroupList* groups = db_list_all_groups();
    if (!groups) {
        send_response(client->socket, 500, "Failed to get groups");
        return;
    }

    struct json_object *response = json_object_new_object();
    struct json_object *payload = json_object_new_object();
    struct json_object *group_array = json_object_new_array();

    for (int i = 0; i < groups->group_count; i++) {
        struct json_object *group = json_object_new_object();
        json_object_object_add(group, "groupName", json_object_new_string(groups->groups[i].group_name));
        json_object_object_add(group, "createdBy", json_object_new_string(groups->groups[i].created_by));
        json_object_object_add(group, "createdAt", json_object_new_string(groups->groups[i].created_at));
        json_object_array_add(group_array, group);
    }

    json_object_object_add(response, "responseCode", json_object_new_int(200));
    json_object_object_add(payload, "listGroups", group_array);
    json_object_object_add(response, "payload", payload);

    const char* response_str = json_object_to_json_string(response);
    send(client->socket, response_str, strlen(response_str), 0);

    json_object_put(response);
    group_list_destroy(groups);
}

void handle_join_group(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);

    if (db_check_is_member(client->username, group_name)) {
        send_response(client->socket, 409, "Already a member of this group");
    } else {
        if (db_request_join(client->username, group_name)) {
            send_response(client->socket, 200, "Join request sent successfully");
        } else {
            send_response(client->socket, 429, "Failed to send join request");
        }
    }

    json_object_put(parsed_json);
}

void handle_invite_to_group(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj, *invited_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    json_object_object_get_ex(payload, "invitedName", &invited_name_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);
    const char* invited_name = json_object_get_string(invited_name_obj);

    if (!db_check_is_member(client->username, group_name)) {
        send_response(client->socket, 403, "Not a member of this group");
    } else if (db_check_is_member(invited_name, group_name)) {
        send_response(client->socket, 409, "User is already a member");
    } else {
        if (db_invite_join(invited_name, group_name)) {
            send_response(client->socket, 200, "Invitation sent successfully");
        } else {
            send_response(client->socket, 400, "Failed to send invitation");
        }
    }

    json_object_put(parsed_json);
}

void handle_leave_group(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);

    if (db_check_is_admin(client->username, group_name)) {
        send_response(client->socket, 403, "Admin cannot leave group");
    } else if (!db_check_is_member(client->username, group_name)) {
        send_response(client->socket, 404, "Not a member of this group");
    } else {
        if (db_leave_group(client->username, group_name)) {
            send_response(client->socket, 200, "Left group successfully");
        } else {
            send_response(client->socket, 501, "Failed to leave group");
        }
    }

    json_object_put(parsed_json);
}

void handle_list_group_members(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);

    if (!db_check_is_member(client->username, group_name)) {
        send_response(client->socket, 403, "Not a member of this group");
        json_object_put(parsed_json);
        return;
    }

    GroupMemberList* members = db_list_members(group_name);
    if (!members) {
        send_response(client->socket, 500, "Failed to get members");
        json_object_put(parsed_json);
        return;
    }

    struct json_object *response = json_object_new_object();
    struct json_object *resp_payload = json_object_new_object();
    struct json_object *member_array = json_object_new_array();

    for (int i = 0; i < members->member_count; i++) {
        json_object_array_add(member_array, 
            json_object_new_string(members->members[i].user_name));
    }

    json_object_object_add(response, "responseCode", json_object_new_int(200));
    json_object_object_add(resp_payload, "listOfMembers", member_array);
    json_object_object_add(response, "payload", resp_payload);

    const char* response_str = json_object_to_json_string(response);
    send(client->socket, response_str, strlen(response_str), 0);

    json_object_put(response);
    json_object_put(parsed_json);
}

void handle_list_invitations(client_t* client, const char* buffer) {
    JoinRequestStatus* status = db_get_invitation_status(client->username);
    if (!status) {
        send_response(client->socket, 201, "No invitations");
        return;
    }

    struct json_object *response = json_object_new_object();
    struct json_object *payload = json_object_new_object();
    struct json_object *invitation_array = json_object_new_array();

    struct json_object *invitation = json_object_new_object();
    json_object_object_add(invitation, "groupName", json_object_new_string(status->group_name));
    json_object_object_add(invitation, "status", json_object_new_string(status->status == JOIN_STATUS_PENDING ? "pending" : 
                                                                       status->status == JOIN_STATUS_ACCEPTED ? "accepted" : "denied"));
    json_object_object_add(invitation, "requestTime", json_object_new_string(status->request_time));
    json_object_array_add(invitation_array, invitation);

    json_object_object_add(response, "responseCode", json_object_new_int(200));
    json_object_object_add(payload, "listOfInvitation", invitation_array);
    json_object_object_add(response, "payload", payload);

    const char* response_str = json_object_to_json_string(response);
    send(client->socket, response_str, strlen(response_str), 0);

    json_object_put(response);
    free(status);
}

void handle_remove_member(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj, *member_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    json_object_object_get_ex(payload, "memberName", &member_name_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);
    const char* member_name = json_object_get_string(member_name_obj);

    if (!db_check_is_admin(client->username, group_name)) {
        send_response(client->socket, 403, "Not an admin of this group");
    } else if (db_check_is_admin(member_name, group_name)) {
        send_response(client->socket, 400, "Cannot remove admin");
    } else if (!db_check_is_member(member_name, group_name)) {
        send_response(client->socket, 404, "Member not found");
    } else {
        if (db_remove_member(member_name, group_name)) {
            send_response(client->socket, 200, "Member removed successfully");
        } else {
            send_response(client->socket, 501, "Failed to remove member");
        }
    }

    json_object_put(parsed_json);
}

void handle_approval(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj, *requester_obj, *decision_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    json_object_object_get_ex(payload, "requester", &requester_obj);
    json_object_object_get_ex(payload, "decision", &decision_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);
    const char* requester = json_object_get_string(requester_obj);
    const char* decision = json_object_get_string(decision_obj);

    if (!db_check_is_admin(client->username, group_name)) {
        send_response(client->socket, 403, "Not an admin of this group");
    } else {
        if (strcmp(decision, "ACCEPT") == 0) {
            if (db_accept_join(requester, group_name)) {
                send_response(client->socket, 200, "Request accepted");
            } else {
                send_response(client->socket, 400, "Failed to accept request");
            }
        } else {
            if (db_deny_join(requester, group_name)) {
                send_response(client->socket, 200, "Request denied");
            } else {
                send_response(client->socket, 400, "Failed to deny request");
            }
        }
    }

    json_object_put(parsed_json);
}

void handle_join_request_status(client_t* client, const char* buffer) {
    JoinRequestStatus* status = db_get_join_status(client->username);
    if (!status) {
        send_response(client->socket, 201, "No join requests");
        return;
    }

    struct json_object *response = json_object_new_object();
    struct json_object *payload = json_object_new_object();
    struct json_object *status_array = json_object_new_array();

    struct json_object *request = json_object_new_object();
    json_object_object_add(request, "groupName", json_object_new_string(status->group_name));
    json_object_object_add(request, "status", json_object_new_string(status->status == JOIN_STATUS_PENDING ? "pending" : 
                                                                    status->status == JOIN_STATUS_ACCEPTED ? "accepted" : "denied"));
    json_object_object_add(request, "requestTime", json_object_new_string(status->request_time));
    json_object_array_add(status_array, request);

    json_object_object_add(response, "responseCode", json_object_new_int(200));
    json_object_object_add(payload, "listOfAppliedGroups", status_array);
    json_object_object_add(response, "payload", payload);

    const char* response_str = json_object_to_json_string(response);
    send(client->socket, response_str, strlen(response_str), 0);

    json_object_put(response);
    free(status);
}

void handle_join_request_list(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);

    if (!db_check_is_admin(client->username, group_name)) {
        send_response(client->socket, 403, "Not an admin of this group");
        json_object_put(parsed_json);
        return;
    }

    JoinRequestList* requests = db_list_join_requests(group_name);
    if (!requests || requests->request_count == 0) {
        send_response(client->socket, 201, "No pending requests");
        if (requests) free(requests);
        json_object_put(parsed_json);
        return;
    }

    struct json_object *response = json_object_new_object();
    struct json_object *resp_payload = json_object_new_object();
    struct json_object *request_array = json_object_new_array();

    for (int i = 0; i < requests->request_count; i++) {
        struct json_object *request = json_object_new_object();
        json_object_object_add(request, "userName", json_object_new_string(requests->requests[i].user_name));
        
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", 
                localtime(&requests->requests[i].request_time));
        json_object_object_add(request, "requestTime", json_object_new_string(time_str));
        
        json_object_array_add(request_array, request);
    }

    json_object_object_add(response, "responseCode", json_object_new_int(200));
    json_object_object_add(resp_payload, "joinRequestList", request_array);
    json_object_object_add(response, "payload", resp_payload);

    const char* response_str = json_object_to_json_string(response);
    send(client->socket, response_str, strlen(response_str), 0);

    json_object_put(response);
    json_object_put(parsed_json);
    free(requests);
}

// Helper function to track progress for file operations
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
static void copy_folder(const char* src_path, const char* dst_path) {
    DIR* dir = opendir(src_path);
    if (!dir) return;

    mkdir(dst_path, 0777);
    
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
            continue;

        char src[2048], dst[2048];
        snprintf(src, sizeof(src), "%s/%s", src_path, entry->d_name);
        snprintf(dst, sizeof(dst), "%s/%s", dst_path, entry->d_name);

        struct stat st;
        if (stat(src, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                copy_folder(src, dst);
            } else {
                FILE *src_file = fopen(src, "rb");
                FILE *dst_file = fopen(dst, "wb");
                if (src_file && dst_file) {
                    char buf[4096];
                    size_t n;
                    while ((n = fread(buf, 1, sizeof(buf), src_file)) > 0) {
                        fwrite(buf, 1, n, dst_file);
                    }
                    fclose(src_file);
                    fclose(dst_file);
                }
            }
        }
    }
    closedir(dir);
}

// Helper function to delete folder contents
static void delete_folder(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
            continue;

        char full_path[2048];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                delete_folder(full_path);
                rmdir(full_path);
            } else {
                unlink(full_path);
            }
        }
    }
    closedir(dir);
    rmdir(path);
}

void handle_folder_content(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj, *folder_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    json_object_object_get_ex(payload, "folderName", &folder_name_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);
    const char* folder_name = json_object_get_string(folder_name_obj);

    if (!db_check_is_member(client->username, group_name)) {
        send_response(client->socket, 403, "Not a member of this group");
        json_object_put(parsed_json);
        return;
    }

    char path[1024];
    snprintf(path, sizeof(path), "root/%s/%s", group_name, folder_name);
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        send_response(client->socket, 404, "Folder not found");
        json_object_put(parsed_json);
        return;
    }

    FolderContents* contents = db_folder_contents(group_name, folder_name);
    if (!contents) {
        send_response(client->socket, 500, "Failed to get folder contents");
        json_object_put(parsed_json);
        return;
    }

    struct json_object *response = json_object_new_object();
    struct json_object *resp_payload = json_object_new_object();
    struct json_object *files_array = json_object_new_array();

    for (int i = 0; i < contents->file_count; i++) {
        struct json_object *file = json_object_new_object();
        json_object_object_add(file, "fileName", json_object_new_string(contents->files[i].file_name));
        json_object_object_add(file, "fileSize", json_object_new_int64(contents->files[i].file_size));
        json_object_array_add(files_array, file);
    }

    json_object_object_add(response, "responseCode", json_object_new_int(200));
    json_object_object_add(resp_payload, "folderContents", files_array);
    json_object_object_add(response, "payload", resp_payload);

    const char* response_str = json_object_to_json_string(response);
    send(client->socket, response_str, strlen(response_str), 0);

    json_object_put(response);
    json_object_put(parsed_json);
    free(contents);
}

void handle_create_folder(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj, *folder_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    json_object_object_get_ex(payload, "folderName", &folder_name_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);
    const char* folder_name = json_object_get_string(folder_name_obj);

    if (!db_check_is_member(client->username, group_name)) {
        send_response(client->socket, 403, "Not a member of this group");
        json_object_put(parsed_json);
        return;
    }

    char path[1024];
    snprintf(path, sizeof(path), "root/%s/%s", group_name, folder_name);
    
    struct stat st = {0};
    if (stat(path, &st) == 0) {
        send_response(client->socket, 409, "Folder already exists");
    } else {
        if (mkdir(path, 0777) == 0) {
            if (db_create_folder(folder_name, group_name)) {
                send_response(client->socket, 201, "Folder created successfully");
            } else {
                rmdir(path);
                send_response(client->socket, 501, "Failed to create folder in database");
            }
        } else {
            send_response(client->socket, 501, "Failed to create folder");
        }
    }

    json_object_put(parsed_json);
}

void handle_folder_rename(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj, *folder_name_obj, *new_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    json_object_object_get_ex(payload, "folderName", &folder_name_obj);
    json_object_object_get_ex(payload, "newFolderName", &new_name_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);
    const char* folder_name = json_object_get_string(folder_name_obj);
    const char* new_name = json_object_get_string(new_name_obj);

    if (!db_check_is_member(client->username, group_name)) {
        send_response(client->socket, 403, "Not a member of this group");
        json_object_put(parsed_json);
        return;
    }

    char old_path[1024], new_path[1024];
    snprintf(old_path, sizeof(old_path), "root/%s/%s", group_name, folder_name);
    snprintf(new_path, sizeof(new_path), "root/%s/%s", group_name, new_name);
    
    struct stat st = {0};
    if (stat(old_path, &st) == -1) {
        send_response(client->socket, 404, "Folder not found");
    } else {
        if (db_rename_folder(group_name, folder_name, new_name)) {
            if (rename(old_path, new_path) == 0) {
                send_response(client->socket, 200, "Folder renamed successfully");
            } else {
                db_rename_folder(group_name, new_name, folder_name); // Rollback
                send_response(client->socket, 501, "Failed to rename folder");
            }
        } else {
            send_response(client->socket, 501, "Failed to rename folder in database");
        }
    }

    json_object_put(parsed_json);
}

void handle_folder_copy(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *from_group_obj, *to_group_obj, *folder_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fromGroup", &from_group_obj);
    json_object_object_get_ex(payload, "toGroup", &to_group_obj);
    json_object_object_get_ex(payload, "folderName", &folder_name_obj);
    
    const char* from_group = json_object_get_string(from_group_obj);
    const char* to_group = json_object_get_string(to_group_obj);
    const char* folder_name = json_object_get_string(folder_name_obj);

    if (!db_check_is_member(client->username, from_group) || 
        !db_check_is_member(client->username, to_group)) {
        send_response(client->socket, 403, "Not a member of both groups");
        json_object_put(parsed_json);
        return;
    }

    char src_path[1024], dst_path[1024];
    snprintf(src_path, sizeof(src_path), "root/%s/%s", from_group, folder_name);
    snprintf(dst_path, sizeof(dst_path), "root/%s/%s", to_group, folder_name);
    
    struct stat st = {0};
    if (stat(src_path, &st) == -1) {
        send_response(client->socket, 404, "Source folder not found");
    } else {
        if (db_copy_folder(from_group, folder_name, to_group)) {
            copy_folder(src_path, dst_path);
            if (stat(dst_path, &st) == 0) {
                send_response(client->socket, 200, "Folder copied successfully");
            } else {
                db_delete_folder(folder_name, to_group); // Rollback
                send_response(client->socket, 501, "Failed to copy folder");
            }
        } else {
            send_response(client->socket, 501, "Failed to copy folder in database");
        }
    }

    json_object_put(parsed_json);
}

void handle_folder_delete(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *group_name_obj, *folder_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "groupName", &group_name_obj);
    json_object_object_get_ex(payload, "folderName", &folder_name_obj);
    
    const char* group_name = json_object_get_string(group_name_obj);
    const char* folder_name = json_object_get_string(folder_name_obj);

    if (!db_check_is_admin(client->username, group_name)) {
        send_response(client->socket, 403, "Not an admin of this group");
        json_object_put(parsed_json);
        return;
    }

    char path[1024];
    snprintf(path, sizeof(path), "root/%s/%s", group_name, folder_name);
    
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        send_response(client->socket, 404, "Folder not found");
    } else {
        if (db_delete_folder(folder_name, group_name)) {
            delete_folder(path);
            send_response(client->socket, 200, "Folder deleted successfully");
        } else {
            send_response(client->socket, 501, "Failed to delete folder");
        }
    }

    json_object_put(parsed_json);
}

void handle_folder_move(client_t* client, const char* buffer) {
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *from_group_obj, *to_group_obj, *folder_name_obj;
    
    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "fromGroup", &from_group_obj);
    json_object_object_get_ex(payload, "toGroup", &to_group_obj);
    json_object_object_get_ex(payload, "folderName", &folder_name_obj);
    
    const char* from_group = json_object_get_string(from_group_obj);
    const char* to_group = json_object_get_string(to_group_obj);
    const char* folder_name = json_object_get_string(folder_name_obj);

    if (!db_check_is_member(client->username, from_group) || 
        !db_check_is_member(client->username, to_group)) {
        send_response(client->socket, 403, "Not a member of both groups");
        json_object_put(parsed_json);
        return;
    }

    char src_path[1024], dst_path[1024];
    snprintf(src_path, sizeof(src_path), "root/%s/%s", from_group, folder_name);
    snprintf(dst_path, sizeof(dst_path), "root/%s/%s", to_group, folder_name);
    
    struct stat st = {0};
    if (stat(src_path, &st) == -1) {
        send_response(client->socket, 404, "Source folder not found");
    } else {
        if (rename(src_path, dst_path) == 0) {
            send_response(client->socket, 200, "Folder moved successfully");
        } else {
            send_response(client->socket, 501, "Failed to move folder");
        }
    }

    json_object_put(parsed_json);
}

// Implement other handlers similarly... 