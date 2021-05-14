#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include "libmemcached/memcached.h"
#include "smc.h"

/**
 * @brief Store data at the memcached server
 * @param ip        Point to the memcached host.
 * @param port      Point to the memcached port 
 * @param timeout   Connecting to memcached timeout(seconds)
 * @param key       The key that will be associated with the item.
 * @param klen      The length of key
 * @param val       The variable to store
 * @param vlen      The length of val
 * @param expire    Expiration time of the item. If it's equal to zero, the item will never expire.
 * @param err_str   The error description
 * @param err_str_size  The memory size of err_str
 * @return 0:succ 1:connect fail 3:other error
 */
int smc_set(char *ip, int port, int timeout, char *key, size_t klen, char *val, size_t vlen, time_t expire, char *err_str, size_t err_str_size)
{
    size_t nval = 0;
    uint32_t flag = 0;
    int ret = 0;
    memcached_st *memc = NULL;
    memcached_server_st *mc_servers = NULL;
    memcached_return mrc;

    memc = memcached_create(NULL);

    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, timeout);

    mc_servers = memcached_server_list_append(NULL, ip, port, &mrc);
    if (MEMCACHED_SUCCESS != mrc)
    {
        snprintf(err_str, err_str_size, "%s", memcached_strerror(memc, mrc));
        ret = SMC_FAIL;
        goto RET_FIN;
    }

    mrc = memcached_server_push(memc, mc_servers);
    if (MEMCACHED_SUCCESS != mrc)
    {
        snprintf(err_str, err_str_size, "memcached_server_push fail:%s", memcached_strerror(memc, mrc));
        ret = SMC_FAIL;
        goto RET_FIN;
    }

    mrc = memcached_set(memc, key, klen, val, vlen, expire, flag);
    if (MEMCACHED_SUCCESS != mrc)
    {
        if (MEMCACHED_CONNECTION_FAILURE == mrc)
            ret = SMC_CONNFAIL;
        else
            ret = SMC_FAIL;
        snprintf(err_str, err_str_size, "memcached_set fail:%s", memcached_strerror(memc, mrc));
        goto RET_FIN;
    }
    else
    {
        ret = SMC_SUCC;
        goto RET_FIN;
    }

RET_FIN:
    if (memc)
        memcached_free(memc);
    if (mc_servers)
        memcached_server_list_free(mc_servers);

    return ret;
}

/**
 * @brief Retrieve item from the server
 * @param ip        Point to the memcached host.
 * @param port      Point to the memcached port 
 * @param timeout   Connecting to memcached timeout(seconds)
 * @param key       The key or array of keys to fetch.
 * @param klen      The length of key
 * @param val       The store value for key
 * @param vsize     The memory size of val
 * @param err_str   The error description
 * @param err_str_size  The memory size of err_str
 * @return 0:succ 1:connect fail 2:notfound 3:fail
 */
int smc_get(char *ip, int port, int timeout, char *key, size_t klen, char *val, size_t vsize, char *err_str, size_t err_str_size)
{
    size_t nval = 0;
    uint32_t flag = 0;
    int ret = 0;
    char *result = NULL;
    memcached_st *memc = NULL;
    memcached_server_st *mc_servers = NULL;
    memcached_return mrc;

    memc = memcached_create(NULL);

    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, timeout);

    mc_servers = memcached_server_list_append(NULL, ip, port, &mrc);
    if (MEMCACHED_SUCCESS != mrc)
    {
        snprintf(err_str, err_str_size, "%s", memcached_strerror(memc, mrc));
        ret = SMC_FAIL;
        goto RET_FIN;
    }

    mrc = memcached_server_push(memc, mc_servers);
    if (MEMCACHED_SUCCESS != mrc)
    {
        snprintf(err_str, err_str_size, "memcached_server_push fail:%s", memcached_strerror(memc, mrc));
        ret = SMC_FAIL;
        goto RET_FIN;
    }

    result = memcached_get(memc, key, klen, (size_t *)&nval, &flag, &mrc);
    if (MEMCACHED_SUCCESS == mrc)
    {
        snprintf(val, vsize, "%s", result);
        ret = SMC_SUCC;
        goto RET_FIN;
    }
    else if (MEMCACHED_CONNECTION_FAILURE == mrc)
    {
        snprintf(err_str, err_str_size, "memcached_get fail:%s", memcached_strerror(memc, mrc));
        ret = SMC_CONNFAIL;
        goto RET_FIN;
    }
    else if (MEMCACHED_NOTFOUND == mrc)
    {
        ret = SMC_NOTFOUND;
        goto RET_FIN;
    }
    else
    {
        snprintf(err_str, err_str_size, "memcached_get fail:%s", memcached_strerror(memc, mrc));
        ret = SMC_FAIL;
        goto RET_FIN;
    }

RET_FIN:
    if (memc)
        memcached_free(memc);
    if (mc_servers)
        memcached_server_list_free(mc_servers);
    if (result)
        free(result);

    return ret;
}

/**
 * @brief Retrieve item from the server
 * @param ip        Point to the memcached host.
 * @param port      Point to the memcached port 
 * @param timeout   Connecting to memcached timeout(seconds)
 * @param key       The key or array of keys to fetch.
 * @param klen      The length of key
 * @param ret       0:succ 1:connect fail 2:notfound 3:fail
 * @param err_str   The error description
 * @param err_str_size  The memory size of err_str
 * @return Returns the value associated with the key, to 
 */
char *smc_get_alloc(char *ip, int port, int timeout, char *key, size_t klen, unsigned int *ret, char *err_str, size_t err_str_size)
{
    size_t nval = 0;
    uint32_t flag = 0;
    char *result = NULL;
    memcached_st *memc = NULL;
    memcached_server_st *mc_servers = NULL;
    memcached_return mrc;

    memc = memcached_create(NULL);

    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, timeout);

    mc_servers = memcached_server_list_append(NULL, ip, port, &mrc);
    if (MEMCACHED_SUCCESS != mrc)
    {
        snprintf(err_str, err_str_size, "%s", memcached_strerror(memc, mrc));
        *ret = SMC_FAIL;
        goto RET_FIN;
    }

    mrc = memcached_server_push(memc, mc_servers);
    if (MEMCACHED_SUCCESS != mrc)
    {
        snprintf(err_str, err_str_size, "memcached_server_push fail:%s", memcached_strerror(memc, mrc));
        *ret = SMC_FAIL;
        goto RET_FIN;
    }

    result = memcached_get(memc, key, klen, (size_t *)&nval, &flag, &mrc);
    if (MEMCACHED_SUCCESS == mrc)
    {
        *ret = SMC_SUCC;
        goto RET_FIN;
    }
    else if (MEMCACHED_CONNECTION_FAILURE == mrc)
    {
        snprintf(err_str, err_str_size, "memcached_get fail:%s", memcached_strerror(memc, mrc));
        *ret = SMC_CONNFAIL;
        goto RET_FIN;
    }
    else if (MEMCACHED_NOTFOUND == mrc)
    {
        *ret = SMC_NOTFOUND;
        goto RET_FIN;
    }
    else
    {
        snprintf(err_str, err_str_size, "memcached_get fail:%s", memcached_strerror(memc, mrc));
        *ret = SMC_FAIL;
        goto RET_FIN;
    }

RET_FIN:
    if (memc)
        memcached_free(memc);
    if (mc_servers)
        memcached_server_list_free(mc_servers);

    return result;
}

/**
 * @brief Delete item from the server
 * @param ip        Point to the memcached host.
 * @param port      Point to the memcached port 
 * @param timeout   Connecting to memcached timeout(seconds)
 * @param key       The key associated with the item to delete.
 * @param klen      The length of key
 * @param err_str   The error description
 * @param err_str_size  The memory size of err_str
 * @return 0:succ 1:connect fail 2:notfound 3:fail
 */
int smc_del(char *ip, int port, int timeout, char *key, size_t klen, char *err_str, size_t err_str_size)
{
    size_t nval = 0;
    uint32_t flag = 0;
    int ret = 0;
    memcached_st *memc = NULL;
    memcached_server_st *mc_servers = NULL;
    memcached_return mrc;

    memc = memcached_create(NULL);

    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
    memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, timeout);

    mc_servers = memcached_server_list_append(NULL, ip, port, &mrc);
    if (MEMCACHED_SUCCESS != mrc)
    {
        snprintf(err_str, err_str_size, "%s", memcached_strerror(memc, mrc));
        ret = SMC_FAIL;
        goto RET_FIN;
    }

    mrc = memcached_server_push(memc, mc_servers);
    if (MEMCACHED_SUCCESS != mrc)
    {
        snprintf(err_str, err_str_size, "memcached_server_push fail:%s", memcached_strerror(memc, mrc));
        ret = SMC_FAIL;
        goto RET_FIN;
    }

    mrc = memcached_delete(memc, key, klen, 0);
    if (MEMCACHED_SUCCESS == mrc)
    {
        ret = SMC_SUCC;
        goto RET_FIN;
    }
    else if (MEMCACHED_CONNECTION_FAILURE == mrc)
    {
        snprintf(err_str, err_str_size, "memcached_delete fail:%s", memcached_strerror(memc, mrc));
        ret = SMC_CONNFAIL;
        goto RET_FIN;
    }
    else if (MEMCACHED_NOTFOUND == mrc)
    {
        ret = SMC_NOTFOUND;
        goto RET_FIN;
    }
    else
    {
        snprintf(err_str, err_str_size, "memcached_delete fail:%s", memcached_strerror(memc, mrc));
        ret = SMC_FAIL;
        goto RET_FIN;
    }

RET_FIN:
    if (memc)
        memcached_free(memc);
    if (mc_servers)
        memcached_server_list_free(mc_servers);

    return ret;
}

#ifdef _TEST
// gcc -g smc.c -lmemcached

char res[5][50] = {"SUCC", "CONNECT FAIL", "NOT FOUND", "FAIL", ""};

void main(int argc, char **argv)
{
    if (argc < 6)
    {
        printf("Usage: %s [type] [ip] [port] [timeout] [key]\n", argv[0]);
        return;
    }

    char err[1024] = {0};
    int ret = -1;
    char *type = argv[1];
    if (strcasecmp(type, "set") == 0)
    {
        if (argc != 8)
        {
            printf("Usage: %s set [ip] [port] [timeout] [key] [val] [expire]\n", argv[0]);
            return;
        }
        char *ip = argv[2];
        char *port = argv[3];
        char *timeout = argv[4];
        char *key = argv[5];
        char *val = argv[6];
        int expire = atoi(argv[7]);

        ret = smc_set(ip, atoi(port), atoi(timeout),
                      key, strlen(key), val, strlen(val),
                      expire, err, sizeof(err));
        printf("smc_set:%s key:%s val:%s\n", res[ret], key, val);
    }
    else if (strcasecmp(type, "get") == 0)
    {
        if (argc != 6)
        {
            printf("Usage: %s get [ip] [port] [timeout] [key]\n", argv[0]);
            return;
        }
        char *ip = argv[2];
        char *port = argv[3];
        char *timeout = argv[4];
        char *key = argv[5];

        char v[1024] = {0};
        ret = smc_get(ip, atoi(port), atoi(timeout),
                      key, strlen(key), v, sizeof(v),
                      err, sizeof(err));
        printf("smc_get:%s key:%s val:%s\n", res[ret], key, v);
    }
    else if (strcasecmp(type, "del") == 0)
    {
        if (argc != 6)
        {
            printf("Usage: %s get [ip] [port] [timeout] [key]\n", argv[0]);
            return;
        }
        char *ip = argv[2];
        char *port = argv[3];
        char *timeout = argv[4];
        char *key = argv[5];

        ret = smc_del(ip, atoi(port), atoi(timeout),
                      key, strlen(key), err, sizeof(err));
        printf("smc_del:%s key:%s\n", res[ret], key);
    }
}
#endif