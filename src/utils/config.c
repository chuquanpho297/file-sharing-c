#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Config *config = NULL;

Config *get_config()
{
    if (config)
    {
        return config;
    }

    config = (Config *)malloc(sizeof(Config));
    if (!config)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    // Get the current build directory
    char cwd[1024];

    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("getcwd() error");
        exit(1);
    }

    char *last_slash = strrchr(cwd, '/');
    *last_slash = '\0';

    load_env_from_file(strcat(cwd, "/.env"));

    load_config(config);

    return config;
}

const char *getenv_or_die(const char *name)
{
    const char *value = getenv(name);
    if (value == NULL)
    {
        fprintf(stderr, "Environment variable %s is not set\n", name);
        exit(1);
    }
    return value;
}

void load_config(Config *config)
{
    config->db_host = getenv_or_die("DB_HOST");
    config->db_user = getenv_or_die("DB_USER");
    config->db_pass = getenv_or_die("DB_PASS");
    config->db_name = getenv_or_die("DB_NAME");
    config->db_port = atoi(getenv_or_die("DB_PORT"));
    config->server_port = atoi(getenv_or_die("SERVER_PORT"));
    config->server_host = getenv_or_die("SERVER_HOST");
    config->root_folder =
        getenv("ROOT_FOLDER") ? getenv("ROOT_FOLDER") : "/root";
    config->temp_folder =
        getenv("TEMP_FOLDER") ? getenv("TEMP_FOLDER") : "/tmp";
}

void load_env_from_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open .env file");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;

        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#')
        {
            continue;
        }

        // Remove "export " prefix if present
        char *key_value = line;
        if (strncmp(line, "export ", 7) == 0)
        {
            key_value = line + 7;
        }

        // Split key and value
        char *delimiter = strchr(key_value, '=');
        if (delimiter)
        {
            *delimiter = '\0';
            const char *key = key_value;
            const char *value = delimiter + 1;

            // Set environment variable
            setenv(key, value, 1);
        }
    }

    fclose(file);
}