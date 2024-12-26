#ifndef SYSTEM_ACCESS_H
#define SYSTEM_ACCESS_H

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zip.h>

// Function prototypes
void copy_file(const char *src_path, const char *dst_path);
void copy_folder(const char *src_path, const char *dst_path);
void copy_all_contents_folder(const char *src_path, const char *dst_path);
void move_all_contents_folder(const char *src_path, const char *dst_path);
void move_file(const char *src_path, const char *dst_path);
void delete_all_contents_folder(const char *src_path);
void delete_file(const char *src_path);
void delete_folder(const char *src_path);
void compress_folder_to_zip(zip_t *zip, const char *folder_path,
                            const char *zip_path);
void create_directories(const char *path);
void compress_folder(const char *folder_path, const char *zip_path);
void compress_folder_to_zip(zip_t *zip, const char *folder_path,
                            const char *base_folder);
void create_directories(const char *path);
void extract_zip(const char *zip_path, const char *dest_path);
bool is_folder_exist(const char *folder_path);
void create_folder_if_not_exists(const char *folder_path);
const char *get_filename(const char *path);
const char *get_folder_name(const char *path);
bool file_exists(const char *filename);
void create_empty_file_if_not_exists(const char *filename);
void read_send_file(int socket, long file_size, FILE *fp);
void receive_write_file(int socket, long file_size, FILE *fp);

#endif  // SYSTEM_ACCESS_H