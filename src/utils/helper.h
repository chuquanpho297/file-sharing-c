#ifndef HELPER_H
#define HELPER_H

void copy_all_contents_folder(const char *src_path, const char *dst_path);
void clear_line();
void copy_file(const char *src_path, const char *dst_path);
void copy_folder(const char *src_path, const char *dst_path);
void move_all_contents_folder(const char *src_path, const char *dst_path);
void move_file(const char *src_path, const char *dst_path);
void delete_all_contents_folder(const char *src_path);
void delete_file(const char *src_path);
void delete_folder(const char *src_path);
void rename_folder(const char *src_path, const char *new_name);
void rename_file(const char *src_path, const char *new_name);
void send_response(int socket, int code, const char *message);

#endif // HELPER_H