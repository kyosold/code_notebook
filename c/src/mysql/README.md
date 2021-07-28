# smysql

封装使用 libmysqlclient 库的操作

## 一. 使用方法

a. 引入头文件

```c
#include "mysql/smysql.h"
```

b. Connect mysql

```c
char *host = "127.0.0.1";
int port = 3306;
char *user = "mysqluser";
char *password = "mysql123";
char *db = "db_user";
int timeout = 3;
char err[1024] = {0};

S_MYSQL_CONN *db = NULL;
db = smysql_init_connect(host, port, user, password, db, timeout,
                        err, sizeof(err));
if (db == NULL) {
    printf("mysql connect fail:%s\n", err);
    return 1;
}
```

c. Query 操作

```c
char *sql = "select * from user_tbl";
SMYSQL_RES *res = NULL;
SMYSQL_ROW row;

res = smysql_query_alloc(db, sql, strlen(sql), 1, err, sizeof(err));
if (res == NUll) {
    printf("mysql query fail:%s\n", err);
    smysql_destroy(db);
    return 1;
}

while ((row = smysql_fetch_row(res)) != NULL) {
    printf("%s %s %s\n", row[0], row[1], row[2]);
}

smysql_free_result(res);
```

d. Update 操作

```c
char *sql = "update user_tbl set name = "kyosold" where uid = 1 limit 1";
int ret = smysql_query(db, sql, strlen(sql), 1, err, sizeof(err));
if (ret != 0) {
    printf("mysql update fail:%s\n", err);
} else {
    printf("affected rows:%d\n", smysql_affected_rows(db));
}
```

e. Escape 操作

```c
char str[1024] = {0};
char estr[get_escape_length(sizeof(str))] = {0};
snprintf(str, sizeof(str), "kyosold'hhxxx");
unsigned long lret = smysql_escape_string(db, estr, str, strlen(str));
printf("str:%s escaped str:%s\n", str, estr);
```

f. Disconnect mysql

```c
smysql_destroy(db);
```

### 编译

指定 libmysqlclient 库，例如:

```bash
-I/usr/include/ -L/usr/lib64/mysql -lmysqlclient
```

## 二. 函数说明

```
S_MYSQL_CONN *smysql_init_connect(char *db_host, unsigned int db_port, char *db_user, char *db_pass, char *db_name, uint32_t timeout, char *err_str, size_t err_str_size)
```

- db_host: 指定 mysql 服务器地址
- db_port: 指定 mysql 服务器端口
- db_user: 指定 mysql 用户名
- db_pass: 指定 mysql 密码
- db_name: 指定 mysql 连接的数据库
- timeout: 指定超时时间, 单位:秒
- err_str: 存储当失败后的错误内容
- err_str_size: 指定 err_str 可以存储的字符串个数
- 返回: 成功返回 S_MYSQL_CONN 对象, 失败返回 NULL

> 连接指定的 mysql 服务器
> 返回的对象需要调用 smysql_destroy() 释放内存

```
SMYSQL_RES *smysql_query_alloc(S_MYSQL_CONN *handle, const char *sql_str, int sql_len, int opts, char *err_str, size_t err_str_size)
```

- handle: 指向 S_MYSQL_CONN 对象
- sql_str: SQL 查询语句
- sql_len: sql_str 字符串长度
- opts: 连接重试 1:重试, 0:不再重试
- err_str: 存放当失败时的错误信息
- err_str_size: 可存放 err_str 的字符串个数
- 返回: 成功返回 SMYSQL_RES 对象, 失败返回 NULL

> 获取一个结果数据

```
SMYSQL_ROW smysql_fetch_row(SMYSQL_RES *result)
```

- result: 指向 SMYSQL_RES 对象，由 smysql_query_alloc 返回
- 返回: 成功返回 MYSQL_ROW 结构, 失败返回 NULL

> 获取枚举数组的结果行

```
void smysql_free_result(SMYSQL_RES *result)
```

- result: 指向 SMYSQL_RES 对象，由 smysql_query_alloc 返回

> 释放由 smysql_query_alloc/smysql_store_result 为结果集分配的内存

```
int smysql_query(S_MYSQL_CONN *handle, const char *sql_str, int sql_len, int opts, char *err_str, size_t err_str_size)
```

- handle: 指向 S_MYSQL_CONN 对象
- sql_str: SQL 查询语句
- sql_len: sql_str 字符串长度
- opts: 连接重试 1:重试, 0:不再重试
- err_str: 存放当失败时的错误信息
- err_str_size: 可存放 err_str 的字符串个数
- 返回: 0 成功, 其它为错误

> 提交一个 mysql query

```
uint64_t smysql_affected_rows(S_MYSQL_CONN *handle)
```

- handle: 指向 S_MYSQL_CONN 对象
- 返回: 影响或检索行数, -1 为失败

> 返回更改的行数
> 由于返回的是无符号值, 所以可以通过将返回值与 (uint64_t)-1 进行比较来检查-1

```
unsigned long smysql_escape_string(S_MYSQL_CONN *handle, char *outstr, char *instr, unsigned long instr_len)
```

- handle: 指向 S_MYSQL_CONN 对象
- outstr: 保存 escaped 字符串，内存大小为: instr_len \* 3 + 1
- instr: 需要被 escape 的字符串
- instr_len: instr 的字符串长度
- 返回: 已编码字符串的长度, 不包括终止的空字节，如果发生错误则为-1

> 转义 mysql_query 中使用的字符串

```
void smysql_destroy(S_MYSQL_CONN *handle)
```

- handle: 需要释放的 S_MYSQL_CONN 对象

> 释放和关闭 mysql
