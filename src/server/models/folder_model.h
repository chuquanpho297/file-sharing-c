#ifndef FOLDER_MODEL_H
#define FOLDER_MODEL_H

typedef struct {
    char* file_name;
    long file_size;
} FileInfo;

typedef struct {
    FileInfo* files;
    int file_count;
    char* folder_name;
} FolderContents;

// Constructor and destructor
FolderContents* folder_contents_create(void);
void folder_contents_destroy(FolderContents* contents);

#endif // FOLDER_MODEL_H 