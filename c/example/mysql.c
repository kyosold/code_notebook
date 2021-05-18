/*
Build:
    gcc -g -o test mysql.c -I/usr/include -L/usr/lib64/mysql -lmysqlclient
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "mysql/mysql.h"

void my_mysql_close(MYSQL *msp)
{
    mysql_close(msp);
    msp = NULL;

    /* https://stackoverflow.com/questions/8554585/mysql-c-api-memory-leak */
    mysql_library_end();
}

int main(int argc, char **argv)
{
    char *mysql_host = argv[1];
    unsigned int mysql_port = atoi(argv[2]);
    char *mysql_user = argv[3];
    char *mysql_pass = argv[4];
    char *mysql_db = argv[5];

    uint32_t timeout = 3;

    int ret = 0;
    MYSQL mysql, *sock;
    MYSQL_RES *res = NULL;
    MYSQL_FIELD *mfd = NULL;
    MYSQL_ROW row;

    mysql_init(&mysql);

    ret = mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    if (ret != 0)
    {
        printf("set mysql connect timeout fail: %s\n", mysql_error(&mysql));
        return 1;
    }

    int conn_total_fail_times = 3;
    int conn_cur_fail_times = 0;
    int conn_fail_sleep = 1;
    while (1)
    {
        if (!(sock = mysql_real_connect(&mysql,
                                        mysql_host,
                                        mysql_user,
                                        mysql_pass,
                                        mysql_db,
                                        mysql_pass,
                                        NULL,
                                        0)))
        {
            printf("[%d] connect mysql fail: %s. server(%s:%d) user(%s) password(%s) db(%s)\n",
                   ++conn_cur_fail_times,
                   mysql_error(&mysql),
                   mysql_host, mysql_port, mysql_user,
                   mysql_pass, mysql_db);
            if (conn_cur_fail_times >= conn_total_fail_times)
            {
                printf("[%d] >= [%d]. we don't retry connect.",
                       conn_cur_fail_times, conn_total_fail_times);
                return 1;
            }
            sleep(conn_fail_sleep);
        }
        else
            break;
    }

    // Select ------------
    char selsql[1024] = "select * from tbl";
    char es_selsql[sizeof(selsql) * 3 + 1] = {0};

    unsigned long es_selsql_len = mysql_real_escape_string(sock, es_selsql, selsql, strlen(selsql));

    if (mysql_query(sock, es_selsql) != 0)
    {
        printf("query sql(%s) fail: %s\n", es_selsql, mysql_error(sock));
        my_mysql_close(sock);
        return 1;
    }

    if (!(res = mysql_store_result(sock)))
    {
        printf("sotre sql data fail: %s\n", es_sql, mysql_error(sock));
        my_mysql_close(sock);
        return 1;
    }

    while ((row = mysql_fetch_row(res)) != NULL)
    {
        printf("0:%s 1:%s\n", row[0], row[1]);
    }

    mysql_free_result(res);

    // Update ------------------
    char upsql[1024] = "update tbl set score = 33 where uid = 21 limit 1";
    char es_upsql[sizeof(upsql) * 3 + 1] = {0};

    unsigned long es_upsql_len = mysql_real_escape_string(sock, es_upsql, upsql, strlen(upsql));

    if (mysql_query(sock, es_upsql) != 0)
    {
        printf("query sql(%s) fail: %s\n", es_upsql, mysql_error(sock));
        my_mysql_close(sock);
        return 1;
    }

    uint64_t affected_rows = mysql_affected_rows(sock);
    printf("Affected rows: %ld\n", affected_rows);

    // Disconnect --------------
    my_mysql_close(sock);

    return 0;
}
