#ifndef USER_CONTROLLER_H
#define USER_CONTROLLER_H

#include <stdbool.h>
#include "../models/user_model.h"

// User operations
bool sign_up(const char* user_name, const char* password);
bool sign_in(const char* user_name, const char* password);
User* get_current_user(void);

#endif // USER_CONTROLLER_H 