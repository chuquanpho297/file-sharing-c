#ifndef SYSTEM_ACCESS_H
#define SYSTEM_ACCESS_H

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Function prototypes
void copy_file(const char *src_path, const char *dst_path);
void copy_folder(const char *src_path, const char *dst_path);
void copy_all_contents_folder(const char *src_path, const char *dst_path);
void move_all_contents_folder(const char *src_path, const char *dst_path);
void move_file(const char *src_path, const char *dst_path);
void delete_all_contents_folder(const char *src_path);
void delete_file(const char *src_path);
void delete_folder(const char *src_path);

#endif  // SYSTEM_ACCESS_H