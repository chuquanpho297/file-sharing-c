#include "folder_model.h"
#include <stdlib.h>
#include <string.h>

FolderContents* folder_contents_create(void) {
    FolderContents* contents = malloc(sizeof(FolderContents));
    if (contents == NULL) return NULL;
    
    contents->files = NULL;
    contents->file_count = 0;
    contents->folder_name = NULL;
    contents->group_name = NULL;
    
    return contents;
}

void folder_contents_destroy(FolderContents* contents) {
    if (contents == NULL) return;
    
    for (int i = 0; i < contents->file_count; i++) {
        free(contents->files[i].file_name);
    }
    free(contents->files);
    free(contents->folder_name);
    free(contents->group_name);
    free(contents);
} 