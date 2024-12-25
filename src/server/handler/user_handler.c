#include "user_handler.h"

#include <json-c/json.h>
#include <mysql/mysql.h>

#include "../../utils/helper.h"
#include "../db/db_access.h"
#include "string.h"

void handle_login(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *username_obj, *password_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "userName", &username_obj);
    json_object_object_get_ex(payload, "password", &password_obj);

    const char *username = json_object_get_string(username_obj);
    const char *password = json_object_get_string(password_obj);

    bool success = db_login(username, password);
    if (success)
    {
        strncpy(client->username, username, MAX_USERNAME - 1);
        client->is_logged_in = 1;
        send_response(client->socket, 200, "Login successful");
    }
    else
    {
        send_response(client->socket, 401, "Invalid username or password");
    }

    json_object_put(parsed_json);
}

void handle_register(client_t *client, const char *buffer)
{
    struct json_object *parsed_json = json_tokener_parse(buffer);
    struct json_object *payload, *username_obj, *password_obj;

    json_object_object_get_ex(parsed_json, "payload", &payload);
    json_object_object_get_ex(payload, "userName", &username_obj);
    json_object_object_get_ex(payload, "password", &password_obj);

    const char *username = json_object_get_string(username_obj);
    const char *password = json_object_get_string(password_obj);

    if (db_create_user(username, password))
    {
        strncpy(client->username, username, MAX_USERNAME - 1);
        client->is_logged_in = 1;
        send_response(client->socket, 201, "Registration successful");
        db_create_root_folder(client->username);
    }
    else
    {
        send_response(client->socket, 409, "User already exists");
    }

    json_object_put(parsed_json);
}