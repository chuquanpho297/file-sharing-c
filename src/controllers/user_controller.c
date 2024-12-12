#include "user_controller.h"
#include "../db/db_access.h"
#include <stdlib.h>
#include <string.h>

static User* current_user = NULL;

bool sign_up(const char* user_name, const char* password) {
    bool success = db_create_user(user_name, password);
    if (success) {
        if (current_user != NULL) {
            user_destroy(current_user);
        }
        current_user = user_create(user_name);
    }
    return success;
}

bool sign_in(const char* user_name, const char* password) {
    bool success = db_login(user_name, password);
    if (success) {
        if (current_user != NULL) {
            user_destroy(current_user);
        }
        current_user = user_create(user_name);
    }
    return success;
}

User* get_current_user(void) {
    return current_user;
} 