#include "db_utils.h"
#include <stdio.h>
#include <stdlib.h>

// Initialize the database connection
MYSQL *db_connect(const char *host, const char *user, const char *password, const char *db_name, unsigned int port)
{
    MYSQL *conn = mysql_init(NULL);

    if (conn == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }

    if (mysql_real_connect(conn, host, user, password, db_name, port, NULL, 0) == NULL)
    {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

    return conn;
}

// Close the database connection
void db_disconnect(MYSQL *conn)
{
    if (conn)
    {
        mysql_close(conn);
    }
}

// Execute an SQL query
MYSQL_RES *db_execute_query(MYSQL *conn, const char *query)
{
    if (mysql_query(conn, query))
    {
        fprintf(stderr, "Query failed: %s\n", mysql_error(conn));
        return NULL;
    }

    return mysql_store_result(conn);
}
