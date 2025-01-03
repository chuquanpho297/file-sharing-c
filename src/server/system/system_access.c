#include "system_access.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <zip.h>

#include "../../utils/config.h"
#include "../../utils/structs.h"

bool copy_file(const char *src_path, const char *dest_path)
{
    FILE *src_file = fopen(src_path, "rb");
    if (!src_file)
    {
        perror("Failed to open source file");
        return false;
    }

    FILE *dest_file = fopen(dest_path, "wb");
    if (!dest_file)
    {
        perror("Failed to open destination file");
        fclose(src_file);
        return false;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes;

    while ((bytes = fread(buffer, 1, BUFFER_SIZE, src_file)) > 0)
    {
        if (fwrite(buffer, 1, bytes, dest_file) != bytes)
        {
            perror("Failed to write to destination file");
            fclose(src_file);
            fclose(dest_file);
            return false;
        }
    }

    fclose(src_file);
    fclose(dest_file);
    return true;
}

bool copy_directory(const char *src_path, const char *dest_path)
{
    DIR *dir = opendir(src_path);
    if (!dir)
    {
        perror("Failed to open source directory");
        return false;
    }

    struct dirent *entry;
    char src_full_path[MAX_PATH_LENGTH];
    char dest_full_path[MAX_PATH_LENGTH];

    // Create the destination directory
    if (mkdir(dest_path, 0755) != 0 && errno != EEXIST)
    {
        perror("Failed to create destination directory");
        closedir(dir);
        return false;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        snprintf(src_full_path, sizeof(src_full_path), "%s/%s", src_path, entry->d_name);
        snprintf(dest_full_path, sizeof(dest_full_path), "%s/%s", dest_path, entry->d_name);

        if (entry->d_type == DT_DIR)
        {
            if (!copy_directory(src_full_path, dest_full_path))
            {
                closedir(dir);
                return false;
            }
        }
        else
        {
            if (!copy_file(src_full_path, dest_full_path))
            {
                closedir(dir);
                return false;
            }
        }
    }

    closedir(dir);
    return true;
}

bool copy_folder(const char *from_folder_path, const char *to_folder_path)
{
    char dest_path[MAX_PATH_LENGTH];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", to_folder_path, strrchr(from_folder_path, '/') + 1);
    return copy_directory(from_folder_path, dest_path);
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

bool compress_folder(const char *folder_path, const char *zip_path)
{
    int errorp;
    zip_t *zip = zip_open(zip_path, ZIP_CREATE | ZIP_TRUNCATE, &errorp);
    if (!zip)
    {
        printf("Failed to create ZIP file: %s\n", zip_path);
        return false;
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

    bool result = compress_folder_to_zip(zip, folder_path, base_folder);

    zip_close(zip);

    return result;
}

bool compress_folder_to_zip(zip_t *zip, const char *folder_path, const char *base_folder)
{
    DIR *dir = opendir(folder_path);
    if (!dir)
    {
        printf("Failed to open directory: %s\n", folder_path);
        closedir(dir);
        return false;
    }

    struct dirent *entry;
    char path[BUFFER_SIZE];
    char zip_path[BUFFER_SIZE];
    FILE *file;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }
            snprintf(path, sizeof(path), "%s/%s", folder_path, entry->d_name);
            snprintf(zip_path, sizeof(zip_path), "%s/%s", base_folder, entry->d_name);
            if (!compress_folder_to_zip(zip, path, zip_path))
            {
                closedir(dir);
                return false;
            }
        }
        else
        {
            snprintf(path, sizeof(path), "%s/%s", folder_path, entry->d_name);
            snprintf(zip_path, sizeof(zip_path), "%s/%s", base_folder, entry->d_name);
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

            if (zip_file_add(zip, zip_path, source, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0)
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

    return true;
}

bool create_directories(const char *path)
{
    char tmp[MAX_PATH_LENGTH];
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
            struct stat st = {0};
            if (stat(tmp, &st) == -1)
            {
                if (mkdir(tmp, 0755) == -1)
                {
                    printf("Failed to create directory: %s\n", tmp);
                    return false;
                }
            }
            *p = '/';
        }
    }
    struct stat st = {0};
    if (stat(tmp, &st) == -1)
    {
        if (mkdir(tmp, 0755) == -1)
        {
            printf("Failed to create directory: %s\n", tmp);
            return false;
        }
    }
    return true;
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

bool is_folder_exist(const char *folder_path) { return opendir(folder_path) != NULL; }

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

void get_last_two_elements(const char *input, char *result1, char *result2, char *delimiter)
{
    char buffer[MAX_PATH_LENGTH];
    strncpy(buffer, input, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    // Find the last two occurrences of '/'
    char *last = strrchr(buffer, *delimiter);
    if (last != NULL)
    {
        *last = '\0';  // Temporarily terminate the string at the last '/'
        last = strrchr(buffer, *delimiter);
        if (last != NULL)
        {
            // Copy the last two elements to the result variables
            strcpy(result2, last + 1);  // Get the last element
            *last = '\0';
            strcpy(result1,
                   buffer + strlen(buffer) - strlen(last + 1));  // Get the second last element
        }
    }
}

const char *get_folder_path(const char *file_path)
{
    const char *last_slash = strrchr(file_path, '/');
    if (last_slash == NULL)
    {
        return NULL;  // No slash found, return NULL
    }
    return strndup(file_path, last_slash - file_path);
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

void receive_write_file(int socket, long file_size, FILE *fp)
{
    char buffer[BUFFER_SIZE];
    long total_received = 0;
    int bytes_received;

    while (total_received < file_size && (bytes_received = recv(socket, buffer, sizeof(buffer), 0)) > 0)
    {
        if (bytes_received < 0)
        {
            perror("Failed to receive data");
            break;
        }

        if (fwrite(buffer, 1, bytes_received, fp) != bytes_received)
        {
            perror("Failed to write data to file");
            break;
        }

        total_received += bytes_received;

        printf("Progress: %ld/%ld bytes\r", total_received, file_size);
        fflush(stdout);

        // Send acknowledgment
        // if (send(socket, &bytes_received, sizeof(bytes_received), 0) < 0)
        // {
        //     perror("Failed to send acknowledgment");
        //     break;
        // }
    }
    printf("\n");
    fclose(fp);
}

void read_send_file(int socket, long file_size, FILE *fp)
{
    char buffer[BUFFER_SIZE];
    int data;
    long byte_send = 0;
    int send_data;

    while ((data = fread(buffer, 1, BUFFER_SIZE, fp)) > 0 && byte_send < file_size)
    {
        if (data < 0)
        {
            perror("Failed to read data");
            break;
        }

        if (send(socket, buffer, data, 0) < 0)
        {
            perror("Failed to send data");
            break;
        }

        // Wait for acknowledgment
        // if (recv(socket, &send_data, sizeof(send_data), 0) < 0)
        // {
        //     perror("Failed to receive acknowledgment");
        //     break;
        // }

        byte_send += data;
        printf("Progress: %ld/%ld bytes\r", byte_send, file_size);
        fflush(stdout);
    }
    printf("\n");
    // fclose(fp);
}

int count_files_in_folder(const char *folder_path)
{
    int entry_count = 0;
    DIR *dir = opendir(folder_path);
    if (!dir)
    {
        printf("Failed to open directory: %s\n", folder_path);
        closedir(dir);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            entry_count++;
        }
        else if (entry->d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                char subfolder_path[MAX_PATH_LENGTH];
                snprintf(subfolder_path, sizeof(subfolder_path), "%s/%s", folder_path, entry->d_name);
                int subfolder_entry_count = count_files_in_folder(subfolder_path);
                if (subfolder_entry_count != -1)
                {
                    entry_count += subfolder_entry_count;
                }
            }
        }
    }

    closedir(dir);
    return entry_count;
}

bool move_folder(const char *from_folder_path, const char *to_folder_path)
{
    if (rename(from_folder_path, to_folder_path) != 0)
    {
        perror("Failed to move folder");
        return false;
    }
    return true;
}

bool remove_directory(const char *path)
{
    DIR *dir = opendir(path);
    size_t path_len = strlen(path);
    bool result = true;

    if (dir)
    {
        struct dirent *entry;

        while (result && (entry = readdir(dir)))
        {
            char *buf;
            size_t len;

            // Skip the names "." and ".." as we don't want to recurse on them
            if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                continue;

            len = path_len + strlen(entry->d_name) + 2;
            buf = malloc(len);

            if (buf)
            {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, entry->d_name);

                if (!stat(buf, &statbuf))
                {
                    if (S_ISDIR(statbuf.st_mode))
                    {
                        // Recursive call for directories
                        result = remove_directory(buf);
                    }
                    else
                    {
                        // Remove regular files
                        result = (unlink(buf) == 0);
                    }
                }
                else
                {
                    result = false;
                }
                free(buf);
            }
            else
            {
                result = false;
            }
        }
        closedir(dir);
    }
    else
    {
        result = false;
    }

    if (result)
    {
        // Remove the empty directory
        result = (rmdir(path) == 0);
    }

    return result;
}

void log_operation(const char *username, const char *operation, const char *path, const char *status)
{
    FILE *log_file = fopen(LOG_FILE, "a");
    if (!log_file)
    {
        printf("Failed to open log file\n");
        return;
    }

    time_t now;
    time(&now);
    char *date = ctime(&now);
    date[strlen(date) - 1] = '\0';  // Remove newline

    fprintf(log_file, "[%s] User: %s, Operation: %s, Path: %s, Status: %s\n", date, username, operation, path, status);
    fclose(log_file);
}