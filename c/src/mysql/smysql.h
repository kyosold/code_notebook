#ifndef _S_MYSQL_H
#define _S_MYSQL_H

#include <stdint.h>
#include <stdlib.h>
#include <mysql/mysql.h>

#define BUFSIZE 1024
#define SMYSQL_CONN_RETRY 0x1
#define SMYSQL_CHARSET_NAME "utf8"
#define SMYSQL_TIMEOUT 3

#define SMYSQL_RES MYSQL_RES
#define SMYSQL_ROW MYSQL_ROW
#define SMYSQL_OPTION_MULTI_STATEMENTS_ON MYSQL_OPTION_MULTI_STATEMENTS_ON
#define SMYSQL_OPTION_MULTI_STATEMENTS_OFF MYSQL_OPTION_MULTI_STATEMENTS_OFF

typedef struct smysql_handle
{
    MYSQL *msp;
    char db_host[BUFSIZE];
    unsigned int db_port;
    char db_user[BUFSIZE];
    char db_pass[BUFSIZE];
    char db_name[BUFSIZE];
    uint32_t timeout;
    char charset_name[BUFSIZE];
} S_MYSQL_CONN;

#define smysql_get_handle(handle) (handle->msp)
#define get_escape_length(size) (size * 3 + 1)

S_MYSQL_CONN *smysql_init_connect(char *db_host, unsigned int db_port, char *db_user, char *db_pass, char *db_name, uint32_t timeout, char *err_str, size_t err_str_size);

int smysql_options(S_MYSQL_CONN *handle, int opts, const void *arg, char *err_str, size_t err_str_size);
int smysql_set_server_option(S_MYSQL_CONN *handle, enum enum_mysql_set_option opts, char *err_str, size_t err_str_size);

unsigned long smysql_escape_string(S_MYSQL_CONN *handle, char *outstr, char *instr, unsigned long instr_len);

MYSQL_RES *smysql_store_result(S_MYSQL_CONN *handle, int opts, char *err_str, size_t err_str_size);
int smysql_query(S_MYSQL_CONN *handle, const char *sql_str, int sql_len, int opts, char *err_str, size_t err_str_size);

SMYSQL_RES *smysql_query_alloc(S_MYSQL_CONN *handle, const char *sql_str, int sql_len, int opts, char *err_str, size_t err_str_size);

SMYSQL_ROW smysql_fetch_row(SMYSQL_RES *result);

uint64_t smysql_affected_rows(S_MYSQL_CONN *handle);

void smysql_free_result(SMYSQL_RES *result);
void smysql_destroy(S_MYSQL_CONN *handle);

#endif