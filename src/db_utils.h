#ifndef DB_UTILS_H
#define DB_UTILS_H

#include <mysql/mysql.h>

// Function to initialize the connection
MYSQL *db_connect(const char *host, const char *user, const char *password, const char *db_name, unsigned int port);

// Function to close the connection
void db_disconnect(MYSQL *conn);

// Function to execute a query
MYSQL_RES *db_execute_query(MYSQL *conn, const char *query);

#endif // DB_UTILS_H
