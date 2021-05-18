#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mysql/errmsg.h>
#include <mysql/mysqld_error.h>
#include "smysql.h"

void _smysql_close(MYSQL *msp)
{
    mysql_close(msp);
    msp = NULL;

    /* https://stackoverflow.com/questions/8554585/mysql-c-api-memory-leak */
    mysql_library_end();
}

/**
 * 
 * @return 0:succ, -1:fail, -2:connect fail
 */
int _smysql_conn(S_MYSQL_CONN *handle, char *err_str, size_t err_str_size)
{
    MYSQL *retmysql;
    MYSQL *msp;
    int retval;

    if (handle == NULL)
        return -1;

    /* init mysql lib */
    msp = mysql_init(NULL);
    if (msp == NULL)
    {
        snprintf(err_str, err_str_size, "out of memory");
        return -1;
    }

    retval = mysql_options(msp, MYSQL_OPT_CONNECT_TIMEOUT, &handle->timeout);
    if (retval != 0)
    {
        _smysql_close(msp);
        return -1;
    }

    retval = mysql_options(msp, MYSQL_SET_CHARSET_NAME, &handle->charset_name);
    if (retval != 0)
    {
        _smysql_close(msp);
        return -1;
    }

    handle->msp = msp;
    retmysql = mysql_real_connect(
        handle->msp,
        handle->db_host,
        handle->db_user,
        handle->db_pass,
        handle->db_name,
        handle->db_port,
        NULL,
        CLIENT_INTERACTIVE | CLIENT_MULTI_STATEMENTS);
    if (retmysql == NULL)
    {
        snprintf(err_str, err_str_size, "%s", mysql_error(smysql_get_handle(handle)));

        _smysql_close(handle->msp);
        handle->msp = NULL;
        return -2;
    }

    mysql_autocommit(handle->msp, 1);

    return 0;
}

/**
 * @brief Open a connection to a MySQL Server
 * @param db_host
 * @param db_port
 * @param db_user
 * @param db_pass
 * @param db_name
 * @param timeout
 * @return Returns a S_MYSQL_CONN handle, to be free use smysql_destroy()
 */
S_MYSQL_CONN *smysql_init_connect(char *db_host, unsigned int db_port, char *db_user, char *db_pass, char *db_name, uint32_t timeout, char *err_str, size_t err_str_size)
{
    S_MYSQL_CONN *handle;

    if (db_host == NULL || db_user == NULL || db_pass == NULL || db_name == NULL)
        return NULL;

    /* create a mysql conn handle */
    handle = (S_MYSQL_CONN *)malloc(sizeof(S_MYSQL_CONN));
    if (handle == NULL)
    {
        snprintf(err_str, err_str_size, "out of memory");
        return NULL;
    }

    /* save the mysql information */
    strncpy(handle->db_host, db_host, BUFSIZE);
    strncpy(handle->db_user, db_user, BUFSIZE);
    strncpy(handle->db_pass, db_pass, BUFSIZE);
    strncpy(handle->db_name, db_name, BUFSIZE);
    strncpy(handle->charset_name, SMYSQL_CHARSET_NAME, BUFSIZE);
    handle->db_port = db_port;
    handle->timeout = timeout;

    /* real connect to mysql server */
    if (_smysql_conn(handle, err_str, err_str_size) != 0)
    {
        free(handle);
        return NULL;
    }

    return handle;
}

/**
 * @brief Release memory and close mysql
 */
void smysql_destroy(S_MYSQL_CONN *handle)
{
    if (handle == NULL)
        return;

    // mysql_close(handle->msp);
    // handle->msp = NULL;
    // mysql_library_end();
    _smysql_close(handle->msp);

    free(handle);
    handle = NULL;

    return;
}

/**
 * @brief Escapes a string for use in a mysql_query
 * @param handle    Point to S_MYSQL_CONN object
 * @param outstr    The Escaped string, size is (instr_len * 3 + 1)
 * @param instr     The unescaped string
 * @param instr_len The length of instr
 * @return The length of the encoded string that is placed into the outstr argument, not including the terminating null byte, or -1 if an error occurs.
 */
unsigned long smysql_escape_string(S_MYSQL_CONN *handle, char *outstr, char *instr, unsigned long instr_len)
{
    return mysql_real_escape_string(handle->msp, outstr, instr, instr_len);
}

/**
 * @brief 
 * @param handle The point to S_MYSQL_CONN struct object
 * @param opts  Retry connect (1:retry, 0:never retry)
 * @return A pointer to a MYSQL_RES result structure with the results. NULL if fail.
 * 
 * For every statement that successfully produces a result set (select,show,describe,
 * explain,check table...). to be free call smysql_free_result()
 */
MYSQL_RES *smysql_store_result(S_MYSQL_CONN *handle, int opts, char *err_str, size_t err_str_size)
{
    MYSQL_RES *result = NULL;
    int err;
    int retry_flag = 0;

    if (handle == NULL)
        return NULL;

    if (handle->msp == NULL)
    {
        if (_smysql_conn(handle, err_str, err_str_size) != 0)
            return NULL;
    }

    for (; handle->msp != NULL;)
    {
        result = mysql_store_result(smysql_get_handle(handle));
        if (result == NULL)
        {
            err = mysql_errno(smysql_get_handle(handle));
            snprintf(err_str, err_str_size, "%s", mysql_error(smysql_get_handle(handle)));

            /* syntax error */
            if (err >= ER_ERROR_FIRST && err <= ER_ERROR_LAST)
                break;

            /* connection error */
            else if (err == CR_SERVER_GONE_ERROR || err == CR_SERVER_LOST)
            {
                if ((opts & SMYSQL_CONN_RETRY) != SMYSQL_CONN_RETRY)
                    retry_flag = 1;

                /* disconnected again, never retry */
                if (retry_flag)
                    break;

                /* reconnect it again */
                retry_flag = 1;
                _smysql_close(handle->msp);

                if (_smysql_conn(handle, err_str, err_str_size) != 0)
                {
                    handle->msp = NULL;
                    return NULL;
                }
                else
                    continue;
            }

            /* other fatal error */
            else
            {
                if (err)
                {
                }
                return NULL;
            }
        }
        else
            break;
    }

    return result;
}

/**
 * @brief Send a MySQL query
 * @param handle        Point to S_MYSQL_CONN object
 * @param sql_str       An SQL query string
 * @param sql_len       The length of sql_str
 * @param opts          Retry connect (1:retry, 0:never retry)
 * @param rerr_str      The error string
 * @param rerr_str_size The size of memory for rerr_str
 * @return 0:success, Nonzero if an error occurred.
 */
int smysql_query(S_MYSQL_CONN *handle, const char *sql_str, int sql_len, int opts, char *err_str, size_t err_str_size)
{
    int retval = 0;
    int err = 0;
    int retry_flag = 0;

    if (handle == NULL || sql_str == NULL)
        return -1;

    if (handle->msp == NULL)
    {
        if (_smysql_conn(handle, err_str, err_str_size) != 0)
            return -1;
    }

    for (;;)
    {
        retval = mysql_real_query(smysql_get_handle(handle), sql_str, sql_len);
        if (retval != 0)
        {
            err = mysql_errno(smysql_get_handle(handle));
            snprintf(err_str, err_str_size, "%s", mysql_error(smysql_get_handle(handle)));

            /* syntax error */
            if (err >= ER_ERROR_FIRST && err <= ER_ERROR_LAST)
            {
                if (err == ER_SYNTAX_ERROR)
                {
                    // sql syntax error
                }
                return err;
            }

            /* connection error */
            else if (err == CR_SERVER_GONE_ERROR || err == CR_SERVER_LOST)
            {
                if ((opts & SMYSQL_CONN_RETRY) != SMYSQL_CONN_RETRY)
                    retry_flag = 1;

                /* Disconnected again, never retry */
                if (retry_flag)
                    break;

                retry_flag = 1;

                /* close for a new connection */
                _smysql_close(handle->msp);

                if (_smysql_conn(handle, err_str, err_str_size) != 0)
                {
                    handle->msp = NULL;
                    return -1;
                }
                else
                    continue;
            }

            /* other fatal error */
            else
                return err;
        }
        else
            break;
    }

    return 0;
}

/**
 * @brief Get result data
 * @param handle        Point to S_MYSQL_CONN object
 * @param sql_str       An SQL query string
 * @param sql_len       The length of sql_str
 * @param opts          Retry connect (1:retry, 0:never retry)
 * @param rerr_str      The error string
 * @param rerr_str_size The size of memory for rerr_str
 * @return A pointer to a MYSQL_RES result structure with the results. NULL if fail.
 * 
 * For every statement that successfully produces a result set (select,show,describe,
 * explain,check table...). to be free call smysql_free_result()
 */
SMYSQL_RES *smysql_query_alloc(S_MYSQL_CONN *handle, const char *sql_str, int sql_len, int opts, char *err_str, size_t err_str_size)
{
    int ret = 0;
    ret = smysql_query(handle, sql_str, sql_len, opts, err_str, err_str_size);
    if (ret != 0)
        return NULL;

    return smysql_store_result(handle, opts, err_str, err_str_size);
}

/**
 * @brief Get a result row as an enumerated array
 * @param result    Point to MYSQL_RES object
 * @return A MYSQL_ROW structure for the next row, or NULL. 
 */
SMYSQL_ROW smysql_fetch_row(SMYSQL_RES *result)
{
    return mysql_fetch_row(result);
}

/**
 * @brief   Frees the memory allocated for a result set 
 *          by smysql_query_alloc, smysql_store_result
 * @param result    Point to MYSQL_RES object
 * @return none
 * 
 * When you are done with a result set, you must free 
 * the memory it uses by calling smysql_free_result()
 */
void smysql_free_result(SMYSQL_RES *result)
{
    mysql_free_result(result);
}

/**
 * @brief Set extra connect options and affect behavior for a connection.
 * @param handle Point to S_MYSQL_CONN object
 * @param opts  The option that you want to set
 * @param arg   The value for the option
 * @return 0:succ, -1,-2:disconnect, other if you specify an unknown option.
 */
int smysql_options(S_MYSQL_CONN *handle, int opts, const void *arg, char *err_str, size_t err_str_size)
{
    if (handle == NULL || arg == NULL)
        return -1;

    if (handle->msp == NULL)
    {
        /* reconnect mysql server */
        if (_smysql_conn(handle, err_str, err_str_size) != 0)
            return -2;
    }

    if (handle->msp != NULL)
        return mysql_options(handle->msp, opts, arg);

    return -2;
}

/**
 * @brief   Enables or disables an option for the connection. 
 * @param handle    Point to S_MYSQL_CONN object
 * @param opts      Enables or disables an option for the connection
 *      SMYSQL_OPTION_MULTI_STATEMENTS_ON: Enable multiple-statement support
 *      SMYSQL_OPTION_MULTI_STATEMENTS_OFF: Disable multiple-statement support
 * @return 0:succ, Nonzero if an error occurred.
 */
int smysql_set_server_option(S_MYSQL_CONN *handle, enum enum_mysql_set_option opts, char *err_str, size_t err_str_size)
{
    if (handle == NULL)
        return -1;

    if (handle->msp == NULL)
    {
        /* reconnect mysql server */
        if (_smysql_conn(handle, err_str, err_str_size) != 0)
            return -2;
    }

    if (handle->msp != NULL)
        return mysql_set_server_option(handle->msp, opts);

    return -2;
}

/**
 * @brief Return the number of rows changed
 * @param handle Point to S_MYSQL_CONN object
 * @return -1:error, Indicates the number of rows affected or retrieved. 
 * 
 * 因为mysql_affected_rows() 返回的是无符号值，所以您可以通过将返回值与(uint64_t)-1（或与 (uint64_t)~0等效）进行比较来检查-1 。
 */
uint64_t smysql_affected_rows(S_MYSQL_CONN *handle)
{
    if (handle == NULL || handle->msp == NULL)
        return -1;

    return mysql_affected_rows(handle->msp);
}

#ifdef _TEST
// gcc -g smysql.c -I/usr/include/ -L/usr/lib64/mysql -lmysqlclient
void main(int argc, char **argv)
{
    if (argc != 7)
    {
        printf("Usage: %s [host] [port] [user] [password] [dataname] [sql]\n", argv[0]);
        return;
    }

    char *host = argv[1];
    char *port = argv[2];
    char *user = argv[3];
    char *pass = argv[4];
    char *dbname = argv[5];
    char *sql = argv[6];

    char err_str[1024] = {0};
    S_MYSQL_CONN *db = NULL;
    SMYSQL_RES *res = NULL;
    SMYSQL_ROW row;
    uint32_t timeout = 3;

    // 连接 ------------------------------------
    db = smysql_init_connect(host, atoi(port), user, pass, dbname, timeout, err_str, sizeof(err_str));
    if (db == NULL)
    {
        printf("mysql init_connect fail: %s\n", err_str);
        return;
    }
    // ---------------------------------------------------------

    // Query select ----------------------------
    res = smysql_query_alloc(db, sql, strlen(sql), 1, err_str, sizeof(err_str));
    if (res == NULL)
    {
        printf("msyql query fail:%s\n", err_str);
        smysql_destroy(db);
        return;
    }

    while ((row = smysql_fetch_row(res)) != NULL)
    {
        printf("%s %s %s\n", row[0], row[1], row[2]);
    }

    smysql_free_result(res);

    // Query update ------------------------------
    char usql[1024] = {0};
    snprintf(usql, sizeof(usql), "update adfunding set fid=69 where uid = 2 limit 1");
    int ret = smysql_query(db, usql, strlen(usql), 1, err_str, sizeof(err_str));
    if (ret != 0)
        printf("mysql query fail:%s\n", err_str);

    printf("affected rows: %d\n", smysql_affected_rows(db));

    // Escape -------------------------------------
    char str[1024] = {0};
    char estr[get_escape_length(sizeof(str))] = {0};
    snprintf(str, sizeof(str), "kyosold'hhxxx");
    unsigned long lret = smysql_escape_string(db, estr, str, strlen(str));
    printf("str:(%s) escaped str:(%s)\n", str, estr);

    // Free memory --------------------------------
    smysql_destroy(db);

    return;
}
#endif
