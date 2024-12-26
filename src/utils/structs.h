#ifndef STRUCTS_H
#define STRUCTS_H

#define MAX_USERNAME 32
#define MAX_PASSWORD 32
#define MAX_CLIENTS 100
#define BUFFER_SIZE 8192
#define MAX_FOLDER_NAME 32
#define MAX_PATH_LENGTH 4096
#define PORT 5555

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