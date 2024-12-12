#ifndef USER_MODEL_H
#define USER_MODEL_H

typedef struct {
    char* user_name;
    char* password_hash;
} User;

// Constructor and destructor
User* user_create(const char* user_name);
void user_destroy(User* user);

#endif // USER_MODEL_H 