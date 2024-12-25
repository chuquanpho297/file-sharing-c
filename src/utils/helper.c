#include "helper.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "structs.h"

void clear_line()
{
    printf("\r\033[K"); // Clear current line
    fflush(stdout);
}

void copy_file(const char *src_path, const char *dst_path)
{
    FILE *src = fopen(src_path, "rb");
    FILE *dst = fopen(dst_path, "wb");

    if (src && dst)
    {
        char buf[8192];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
        {
            fwrite(buf, 1, n, dst);
        }
    }

    if (src)
        fclose(src);
    if (dst)
        fclose(dst);
}

void copy_folder(const char *src_path, const char *dst_path)
{
    DIR *dir = opendir(src_path);
    if (!dir)
        return;

    mkdir(dst_path, 0777);

    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char src[2048], dst[2048];
        snprintf(src, sizeof(src), "%s/%s", src_path, entry->d_name);
        snprintf(dst, sizeof(dst), "%s/%s", dst_path, entry->d_name);

        struct stat st;
        if (stat(src, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                copy_folder(src, dst);
            }
            else
            {
                copy_file(src, dst);
            }
        }
    }
    closedir(dir);
}

void copy_all_contents_folder(const char *src_path, const char *dst_path)
{
    DIR *dir = opendir(src_path);
    if (!dir)
        return;

    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char src[2048], dst[2048];
        snprintf(src, sizeof(src), "%s/%s", src_path, entry->d_name);
        snprintf(dst, sizeof(dst), "%s/%s", dst_path, entry->d_name);

        struct stat st;
        if (stat(src, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                mkdir(dst, 0777);
                copy_all_contents_folder(src, dst);
            }
            else
            {
                copy_file(src, dst);
            }
        }
    }
    closedir(dir);
}

void move_all_contents_folder(const char *src_path, const char *dst_path)
{
    copy_all_contents_folder(src_path, dst_path);
    delete_all_contents_folder(src_path);
}

void move_file(const char *src_path, const char *dst_path)
{
    if (rename(src_path, dst_path) != 0)
    {
        // If rename fails (e.g., across devices), fallback to copy+delete
        copy_file(src_path, dst_path);
        delete_file(src_path);
    }
}

void delete_all_contents_folder(const char *src_path)
{
    DIR *dir = opendir(src_path);
    if (!dir)
        return;

    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[2048];
        snprintf(path, sizeof(path), "%s/%s", src_path, entry->d_name);

        struct stat st;
        if (stat(path, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                delete_folder(path);
            }
            else
            {
                delete_file(path);
            }
        }
    }
    closedir(dir);
}

void delete_file(const char *src_path)
{
    unlink(src_path);
}

void delete_folder(const char *src_path)
{
    delete_all_contents_folder(src_path);
    rmdir(src_path);
}

void send_response(int socket, int code, const char *message)
{
    char response[BUFFER_SIZE];
    struct json_object *jobj = json_object_new_object();
    struct json_object *jpayload = json_object_new_object();
    json_object_object_add(jobj, "responseCode", json_object_new_int(code));
    json_object_object_add(jpayload, "message", json_object_new_string(message));
    json_object_object_add(jobj, "payload", jpayload);

    const char *response_str = json_object_to_json_string(response);
    send(socket, response_str, strlen(response_str), 0);

    json_object_put(jobj);
}
