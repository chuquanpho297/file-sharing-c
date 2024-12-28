#include "./config.h";

#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct
{
    int socket;
    char username[MAX_USERNAME];
    int is_logged_in;
} client_t;

typedef struct
{
    char* file_id;
    char* folder_id;
    char* file_name;
    long file_size;
    char* folder_name;
    char* created_by;
    char* created_at;
    char* access;
    char* file_path;
} FileStruct;

typedef struct
{
    FileStruct* files;
    int count;
} FileList;

typedef struct
{
    char* folder_id;
    char* folder_name;
    char* created_by;
    char* parent_folder_id;
    char* folder_path;
    char* created_at;
    char* access;
} FolderStruct;

typedef struct
{
    FolderStruct* folders;
    int count;
} FolderList;

void free_file_list(FileList* file_list);
void free_folder_list(FolderList* folder_list);

#endif