# str_trim

去掉字符串的开头和结尾的字符串

## 使用方法

```c
#include "string/str_trim.h"

char *str = "   abc   ";
char *sed = " ";

// 第一种使用方法
char res[1024] = {0};
str_trim(str, strlen(str), sed, strlen(sed), res, sizeof(res));
printf("%s\n", res);

// 第二种使用方法
char *ss = str_trim_alloc(str, strlen(str), NULL, 0);
printf("%s\n", ss);
free(ss);
```

## 函数说明

```
void str_trim(char *str, int str_len,
            char *what, int what_len,
            char *str_trimmed, int str_trimmed_size)
```

- str: 需要被剪切的字符串
- str_len: 需要被剪切的字符串的长度
- what: 指定要去掉的字符串，NULL 为默认 (' \t\n\r\v\0')
- what_len: what 的长度
- str_trimmed: 保存剪切后的字符串
- str_trimmed_size: str_trimmed 的内存空间

> 指定字符串去掉开头和结尾的空格(或其它字符)

```
void str_ltrim(char *str, int str_len,
            char *what, int what_len,
            char *str_trimmed, int str_trimmed_size)
```

> 与上面相同，但只去掉左侧开头的字符

```
void str_rtrim(char *str, int str_len,
            char *what, int what_len,
            char *str_trimmed, int str_trimmed_size)
```

> 与上面相同，但只去年右侧结尾的字符

```
char *str_trim_alloc(char *str, int str_len, char *what, int what_len)
```

- str: 需要被剪切的字符串
- str_len: 需要被剪切的字符串的长度
- what: 指定要去掉的字符串，NULL 为默认 (' \t\n\r\v\0')
- what_len: what 的长度
- 返回: 剪切后的字符串, 需要手动 free()

> 指定字符串去掉开头和结尾的空格(或其它字符), 需手动 free()

```
char *str_ltrim_alloc(char *str, int str_len, char *what, int what_len)
```

> 与上面相同，但只去掉左侧开头的字符，需手动 free()

```
char *str_rtrim_alloc(char *str, int str_len, char *what, int what_len)
```

> 与上面相同，但只去掉右侧结尾的字符，需手动 free()

# str_addslashes

给字符串特殊字符添加转义字符

## 使用方法

```c
#include "string/str_utils.h"

char *s = "kyosold'apple";
char str[1024] = {0};
int r = str_addslashes(s, strlen(s), str, sizeof(str));
printf("ret:%d str:%s\n", r, str);
```

## 函数说明

```
int str_addslashes(char *str, int len, char *ret_str)
```

- str: 需要被转义的字符串
- len: 被转义的字符串的长度
- ret_str: 保存转义后的字符串 （内存应该是 2 倍的 str 大小）
- 返回: 转义后的字符串的长度

> 使用斜杠 \ 转义特殊字符:
> 特殊字符为:
>
> - 单引号 (')
> - 双引号 (")
> - 反斜杠 (\)
> - NUL (the NUL byte)
