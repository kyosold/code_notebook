#ifndef _S_MC_H
#define _S_MC_H

#define SMC_SUCC 0
#define SMC_CONNFAIL 1
#define SMC_NOTFOUND 2
#define SMC_FAIL 3

int smc_set(char *ip, int port, int timeout, char *key, size_t klen, char *val, size_t vlen, time_t expire, char *err_str, size_t err_str_size);

int smc_get(char *ip, int port, int timeout, char *key, size_t klen, char *val, size_t vsize, char *err_str, size_t err_str_size);

char *smc_get_alloc(char *ip, int port, int timeout, char *key, size_t klen, unsigned int *ret, char *err_str, size_t err_str_size);
int smc_del(char *ip, int port, int timeout, char *key, size_t klen, char *err_str, size_t err_str_size);

#endif
