# spcre

封装 libpcre 正则库操作

## 一. 使用方法

a. 引入头文件

```c
#include "pcre/spcre.h"
```

b. 是否命中，并显示出命中的是哪些字符

```c
char *pattern = "abc((.*)cf(exec))test";
char *str = "11111abcword1cfexectest11111";

int is_matched = SPCRE_NOMATCH;
char **ml = NULL;
size_t ml_num = 0;
char err[1024] = {0};

is_matched = s_pcre_match_alloc(str, strlen(str),
                            pattern,
                            &ml, &ml_num,
                            err, sizeof(err));
if (is_matched == SPCRE_MATCHED) {
    printf("ret: MATCHED\n");

    int i = 0;
    for (i=0; i<ml_num; i++) {
        printf("Matched: [%d]:%s\n", i, ml[i]);
    }

    s_pcre_matched_free(ml, ml_num);
} else if (is_matched == SPCRE_NOMATCH) {
    printf("ret: NO MATCH\n");
    return 1;
} else {
    printf("match error:%s\n", err);
    return 1;
}
```

c. 只判断是否命中

```c
char *pattern = "abc((.*)cf(exec))test";
char *str = "11111abcword1cfexectest11111";

int is_matched = SPCRE_NOMATCH;
char err[1024] = {0};

is_matched = s_pcre_match(str, strlen(str),
                        pattern,
                        err, sizeof(err));
if (is_matched == SPCRE_MATCHED)
    printf("ret: MATCHED\n");
else if (is_matched == SPCRE_NOMATCH)
    printf("ret: NO MATCHED\n");
else
    printf("match error:%s\n", err);
```

## 二. 编译

```bash
-I/usr/include/ -L/usr/lib64/ -lpcre
```

## 三. 函数说明

#### 匹配正则，并获取匹配到的字符串

```
int s_pcre_match_alloc(const char *str, size_t str_len,
                const char *pattern,
                char ***matched_list, size_t *matched_list_num,
                char *err_str, size_t err_str_size)
```

- str: 被匹配的字符串
- str_len: 被匹配的字符串的长度
- pattern: 正则表达式
- matched_list: 命中后的字符串，命中后才会分配内存，需要 s_pcre_matched_free() 释放
- matched_list_num: 命中后的字符串的数量
- err_str: 出错后存储错误信息
- err_str_size: 存储错误信息的 buffer 的空间大小
- 返回: 0 命中, 1 未命中, 2 错误

> 如果命中，则 matched_list 将分配堆上内存，用完后需手动 s_pcre_matched_free 它
> 如果未命中，则此值为 NULL。

`如果你仅需要知道是否命中，而不关心命中的是哪个字符串，则使用 s_pcre_match() 函数`

#### 释放命中的数组列表内存

```
void s_pcre_matched_free(char **matched_list, size_t matched_list_num)
```

- matched_list: 需要被释放的命中数组
- matched_list_num: 数组成员的数量

> 释放命中的数组列表内存

#### 匹配正则表达式

```
int s_pcre_match(const char *str, size_t str_len,
                const char *pattern,
                char *err_str, size_t err_str_size)
```

- str: 被匹配的字符串
- str_len: 被匹配的字符串的长度
- pattern: 正则表达式
- err_str: 出错后存储错误信息
- err_str_size: 存储错误信息的 buffer 的空间大小
- 返回: 0 命中, 1 未命中, 2 错误
