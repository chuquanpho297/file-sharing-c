#ifndef SYSTEM_ACCESS_H
#define SYSTEM_ACCESS_H

#include <arpa/inet.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zip.h>

// Function prototypes
bool copy_file(const char *src_path, const char *dest_path);
bool copy_directory(const char *src_path, const char *dest_path);
bool copy_folder(const char *from_folder_path, const char *to_folder_path);
void copy_all_contents_folder(const char *src_path, const char *dst_path);
void move_all_contents_folder(const char *src_path, const char *dst_path);
void move_file(const char *src_path, const char *dst_path);
void delete_all_contents_folder(const char *src_path);
void delete_file(const char *src_path);
void delete_folder(const char *src_path);
bool create_directories(const char *path);
bool compress_folder(const char *folder_path, const char *zip_path);
bool compress_folder_to_zip(zip_t *zip, const char *folder_path,
                            const char *base_folder);
void extract_zip(const char *zip_path, const char *dest_path);
bool is_folder_exist(const char *folder_path);
void create_folder_if_not_exists(const char *folder_path);
const char *get_filename(const char *path);
const char *get_folder_name(const char *path);
bool file_exists(const char *filename);
void create_empty_file_if_not_exists(const char *filename);
void read_send_file(int socket, long file_size, FILE *fp);
void receive_write_file(int socket, long file_size, FILE *fp);
int count_files_in_folder(const char *folder_path);
void get_last_two_elements(const char *input, char *result1, char *result2,
                           char *delimiter);
const char *get_folder_path(const char *file_path);
bool move_folder(const char *from_folder_path, const char *to_folder_path);

#endif  // SYSTEM_ACCESS_H