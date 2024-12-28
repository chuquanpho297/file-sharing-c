#ifndef CONFIG_H
#define CONFIG_H

typedef struct
{
    const char *db_host;
    const char *db_user;
    const char *db_pass;
    const char *db_name;
    int db_port;
    const char *root_folder;
    const char *temp_folder;
    int server_port;
    const char *server_host;
} Config;

#define CLIENT_ERROR "-1"
#define MAX_USERNAME 32
#define MAX_PASSWORD 32
#define MAX_CLIENTS 100
#define BUFFER_SIZE 8192
#define MAX_FOLDER_NAME 32
#define MAX_PATH_LENGTH 4096
#define MAX_COMMAND_LENGTH 1024
#define LOG_FILE "file-sharing.log"

Config *get_config();
const char *getenv_or_die(const char *name);
void load_config(Config *config);
void load_env_from_file(const char *filename);

#endif  // CONFIG_H