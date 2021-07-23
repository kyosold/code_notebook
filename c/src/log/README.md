# slog

日志函数, 此函数操作 syslog

## 一. 使用方法

a. 引入头文件

```c
#include "log/slog.h"
```

b. 打开日志

```c
slog_open("test", LOG_MAIL, SLOG_DEBUG, "9999999");
```

c. 设定 uid

```c
slog_set_uid("xxxxxxxx");
```

d. 设定日志等级

```c
slog_set_level(SLOG_INFO);
```

e. 写日志

```c
slog_debug("Name: %s", "111111111");
slog_info("Name: %s", "22222222");
slog_error("NAME: %s", "33333333");
```

## 二. 函数说明

```
void slog_open(const char *ident, int facility, int level, char *uid)
```

- ident: 程序名称
- facility: 目标日志 (LOG_MAIL|LOG_USER)
- level: 日志等级 (SLOG_DEBUG, SLOG_INFO ...)
- uid: 日志 UID， 用于同一次会话的标记

```
void slog_set_level(int level)
```

- level: 设置日志等级

> SLOG_DEBUG, SLOG_INFO, SLOG_NOTICE, SLOG_WARNING
> SLOG_ERROR, SLOG_CRIT, SLOG_ALERT, SLOG_EMERG

```
void slog_set_uid(char *uid)
```

- uid: 日志 UID，用于同一次会话的标记
