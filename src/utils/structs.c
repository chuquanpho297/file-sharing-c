#include "structs.h"

#include <stdlib.h>

// Function to free memory allocated for FileList
void free_file_list(FileList *file_list) {
    if (file_list == NULL) {
        return;
    }

    for (int i = 0; i < file_list->count; i++) {
        free(file_list->files[i].file_id);
        free(file_list->files[i].folder_id);
        free(file_list->files[i].file_name);
        free(file_list->files[i].folder_name);
        free(file_list->files[i].created_by);
        free(file_list->files[i].created_at);
        free(file_list->files[i].access);
    }

    free(file_list->files);
    file_list->files = NULL;
    file_list->count = 0;
}

// Function to free memory allocated for FolderList
void free_folder_list(FolderList *folder_list) {
    if (folder_list == NULL) {
        return;
    }

    for (int i = 0; i < folder_list->count; i++) {
        free(folder_list->folders[i].folder_id);
        free(folder_list->folders[i].folder_name);
        free(folder_list->folders[i].created_by);
        free(folder_list->folders[i].parent_folder_id);
        free(folder_list->folders[i].created_at);
        free(folder_list->folders[i].access);
    }

    free(folder_list->folders);
    folder_list->folders = NULL;
    folder_list->count = 0;
}