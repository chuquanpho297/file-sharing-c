#include "user_model.h"
#include <stdlib.h>
#include <string.h>

User* user_create(const char* user_name) {
    User* user = malloc(sizeof(User));
    if (user == NULL) return NULL;
    
    user->user_name = strdup(user_name);
    user->password_hash = NULL;
    
    return user;
}

void user_destroy(User* user) {
    if (user == NULL) return;
    
    free(user->user_name);
    free(user->password_hash);
    free(user);
} 