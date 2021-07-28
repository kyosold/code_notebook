# smc

封装使用 libmemcached 库操作 memcached 的函数

## 一. 使用方法

a. 引入头文件

```c
#include "mc/mc.h"
```

b. Set 操作

```c
char res[5][50] = {"SUCC", "CONNECT FAIL", "NOT FOUND", "FAIL", ""};

char *ip = "127.0.0.1";
int port = 11211;
int timeout = 1;
char *key = "name";
char *value = "kyosold@qq.com";
int expire = 30;
char err[1024] = {0};

int ret = smc_set(ip, port, timeout,
                key, strlen(key), value, strlen(value), expire,
                err, sizeof(err));
if (ret != 0) {
    printf("set mc fail:%s:%s\n", res[ret], err);
    return 1;
}
printf("set mc %s\n", res[ret]);
```

c. Get 操作

```c
char res[5][50] = {"SUCC", "CONNECT FAIL", "NOT FOUND", "FAIL", ""};

char *ip = "127.0.0.1";
int port = 11211;
int timeout = 1;
char *key = "name";
char value[1024] = {0};
char err[1024] = {0};

int ret = smc_get(ip, port, timeout,
                key, strlen(key), value, sizeof(value),
                err, sizeof(err));
if (ret != 0) {
    printf("get mc fail:%s:%s for key:%s\n", res[ret], err, key);
    return 1;
}
printf("get mc ret:%s key:%s value:%s\n", res[ret], key, value);
```

d. del 操作

```c
char res[5][50] = {"SUCC", "CONNECT FAIL", "NOT FOUND", "FAIL", ""};

char *ip = "127.0.0.1";
int port = 11211;
int timeout = 1;
char *key = "name";
char err[1024] = {0};

int ret = smc_del(ip, port, timeout,
                key, strlen(key), err, sizeof(err));
if (ret != 0) {
    printf("del mc fail:%s:%s key:%s\n", rets[ret], err, key);
    return 1;
}
printf("del mc ret:%s key:%s\n", rets[ret], key);
```

## 编译

需要指定 libmemcached 库, 例如:

```bash
-I/usr/local/include/libmemcached/ -L/usr/local/lib64/ -lmemcached
```

## 二. 函数说明

```
int smc_set(char *ip, int port, int timeout, char *key, size_t klen, char *val, size_t vlen, time_t expire, char *err_str, size_t err_str_size)
```

- ip: memcached 服务器 IP 地址
- port: memcached 服务器端口
- timeout: 连接 memcached 服务器超时时间，单位:秒
- key: 存储的 key
- klen: key 的长度
- val: 对应 key 的值
- vlen: val 的长度
- expire: 存储的 key/value 的过期时长，如果是 0 则永不过期，单位:秒
- err_str: 保存执行出错时的字符串
- err_str_size: 指定 err_str 能保存多少个字符
- 返回: 0 成功, 1 连接失败, 3 其它错误

> 存储数据到 memcached 服务器中

```
int smc_get(char *ip, int port, int timeout, char *key, size_t klen, char *val, size_t vsize, char *err_str, size_t err_str_size)
```

- ip: memcached 服务器 IP 地址
- port: memcached 服务器端口
- timeout: 连接 memcached 服务器超时时间，单位:秒
- key: 需要检索的 key
- klen: key 的长度
- val: 保存获取 key 的值
- vlen: 指定 val 可以保存的多少个字符
- err_str: 保存执行出错时的字符串
- err_str_size: 指定 err_str 能保存多少个字符
- 返回: 0 成功, 1 连接失败, 2 未找到, 3 其它错误

> 从 memcached 中检索指定的 key 的值

```
int smc_del(char *ip, int port, int timeout, char *key, size_t klen, char *err_str, size_t err_str_size)
```

- ip: memcached 服务器 IP 地址
- port: memcached 服务器端口
- timeout: 连接 memcached 服务器超时时间，单位:秒
- key: 需要删除的 key
- klen: key 的长度
- err_str: 保存执行出错时的字符串
- err_str_size: 指定 err_str 能保存多少个字符
- 返回: 0 成功, 1 连接失败, 2 未找到, 3 其它错误

> 从 memcached 中删除指定的 key
