#include "system_access.h"

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <zip.h>

#include "../../utils/structs.h"

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

void delete_file(const char *src_path) { unlink(src_path); }

void delete_folder(const char *src_path)
{
    delete_all_contents_folder(src_path);
    rmdir(src_path);
}

void compress_folder(const char *folder_path, const char *zip_path)
{
    int errorp;
    zip_t *zip = zip_open(zip_path, ZIP_CREATE | ZIP_TRUNCATE, &errorp);
    if (!zip)
    {
        printf("Failed to create ZIP file: %s\n", zip_path);
        return;
    }

    const char *base_folder = strrchr(folder_path, '/');
    if (base_folder)
    {
        base_folder++;  // Skip the '/'
    }
    else
    {
        base_folder = folder_path;
    }

    compress_folder_to_zip(zip, folder_path, base_folder);

    zip_close(zip);
}

void compress_folder_to_zip(zip_t *zip, const char *folder_path,
                            const char *base_folder)
{
    DIR *dir = opendir(folder_path);
    if (!dir)
    {
        printf("Failed to open directory: %s\n", folder_path);
        return;
    }

    struct dirent *entry;
    char path[BUFFER_SIZE];
    char zip_path[BUFFER_SIZE];
    FILE *file;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }
            snprintf(path, sizeof(path), "%s/%s", folder_path, entry->d_name);
            snprintf(zip_path, sizeof(zip_path), "%s/%s", base_folder,
                     entry->d_name);
            compress_folder_to_zip(zip, path, zip_path);
        }
        else
        {
            snprintf(path, sizeof(path), "%s/%s", folder_path, entry->d_name);
            snprintf(zip_path, sizeof(zip_path), "%s/%s", base_folder,
                     entry->d_name);
            file = fopen(path, "rb");
            if (!file)
            {
                printf("Failed to open file: %s\n", path);
                continue;
            }

            zip_source_t *source = zip_source_file(zip, path, 0, 0);
            if (source == NULL)
            {
                printf("Failed to add file to ZIP: %s\n", path);
                fclose(file);
                continue;
            }

            if (zip_file_add(zip, zip_path, source,
                             ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0)
            {
                printf("Failed to add file to ZIP: %s\n", path);
                zip_source_free(source);
                fclose(file);
                continue;
            }

            fclose(file);
        }
    }

    closedir(dir);
}

void create_directories(const char *path)
{
    char tmp[BUFFER_SIZE];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    if (tmp[len - 1] == '/')
    {
        tmp[len - 1] = 0;
    }
    for (char *p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

void extract_zip(const char *zip_path, const char *dest_path)
{
    int err;
    zip_t *zip = zip_open(zip_path, 0, &err);
    if (!zip)
    {
        printf("Failed to open ZIP file: %s\n", zip_path);
        return;
    }

    zip_int64_t num_entries = zip_get_num_entries(zip, 0);
    for (zip_int64_t i = 0; i < num_entries; i++)
    {
        const char *name = zip_get_name(zip, i, 0);
        if (!name)
        {
            printf("Failed to get name for entry %ld\n", i);
            continue;
        }

        char full_path[BUFFER_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/%s", dest_path, name);

        if (name[strlen(name) - 1] == '/')
        {
            // Directory
            create_directories(full_path);
        }
        else
        {
            // File
            char *last_slash = strrchr(full_path, '/');
            if (last_slash)
            {
                *last_slash = '\0';
                create_directories(full_path);
                *last_slash = '/';
            }

            zip_file_t *zf = zip_fopen_index(zip, i, 0);
            if (!zf)
            {
                printf("Failed to open file in ZIP: %s\n", name);
                continue;
            }

            FILE *f = fopen(full_path, "wb");
            if (!f)
            {
                printf("Failed to open file for writing: %s\n", full_path);
                zip_fclose(zf);
                continue;
            }

            char buffer[BUFFER_SIZE];
            zip_int64_t bytes_read;
            while ((bytes_read = zip_fread(zf, buffer, sizeof(buffer))) > 0)
            {
                fwrite(buffer, 1, bytes_read, f);
            }

            fclose(f);
            zip_fclose(zf);
        }
    }

    zip_close(zip);
}

bool is_folder_exist(const char *folder_path)
{
    return opendir(folder_path) != NULL ? true : false;
}

void create_folder_if_not_exists(const char *folder_path)
{
    if (!is_folder_exist(folder_path))
    {
        mkdir(folder_path, 0777);
    }
}

const char *get_filename(const char *path)
{
    char *filename = strrchr(path, '/');
    if (filename == NULL)
    {
        return path;  // No '/' found, return the original path
    }
    return filename + 1;  // Skip the '/' character
}

const char *get_folder_name(const char *path)
{
    size_t len = strlen(path);
    if (len > 1 && path[len - 1] == '/')
    {
        path = strndup(path, len - 1);
    }

    const char *last_slash = strrchr(path, '/');
    if (last_slash == NULL)
    {
        return path;
    }

    return last_slash + 1;
}

// Check if file exists
bool file_exists(const char *filename) { return access(filename, F_OK) != -1; }

// Create an empty file if it does not exist
void create_empty_file_if_not_exists(const char *filename)
{
    if (!file_exists(filename))
    {
        FILE *fp = fopen(filename, "wb");
        if (fp != NULL)
        {
            fclose(fp);
        }
        else
        {
            perror("Failed to create file");
        }
    }
}
