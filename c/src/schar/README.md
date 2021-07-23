# schar

封装的字符串，为了方便使用 char 指针的字符串操作。

## 一. 使用方法

a. 引用头文件

```c
#include "schar/schar.h"
```

b. 创建 schar 对象

```c
schar *ss = schar_new();
```

c. 复制前 n 个字符到 schar 中

```c
schar_copyb(ss, "123qwe", 6);
```

d. 追加前 n 个字符到 schar 中

```c
schar_catb(ss, " | ", 3);
```

e. 追加字符串到 schar 中

```c
schar_cats(ss, "ADFB");
```

f. 子字符串查找

```c
int pos = schar_stripos(ss, "fb", 2, 0);

char *s = schar_stristr_alloc(ss, "fb", 2, 0, 1);
free(s);
```

g. 清理 schar

```c
schar_delete(ss);
```

## 二. 函数说明

```
schar *schar_new()
```

- 返回: 失败返回 NULL

> 初始并生成一个 schar 结构指针, 调用 schar_delete() 去释放内存

```
void schar_delete(schar *x)
```

- x: 需要被释放的 schar

> 释放 schar 对象, schar 内 s 指向的内存也一并会释放掉

```
int schar_copyb(schar *dest, char *src, unsigned int n)
```

- dest: 目标 schar 对象
- src: 需要被复制的字符串
- n: 需要被复制的字符串前 n 个
- 返回: 0 成功, 1 失败

> 复制 src 前 n 个字节到 dest 里，并以 \0 结束。
> 如果目标 dest 内存不够，dest 会自动扩展内存并复制。

```
int schar_copys(schar *dest, char *str)
```

- dest: 目标 schar 对象
- src: 被复制的字符串
- 返回: 0 成功, 1 失败

> 复制 str 到 dest 里，并以 \0 结束。
> 它其实是 schar_copyb(dest, str, strlen(str))

```
int schar_copy(schar *dest, schar *source)
```

- dest: 目标 schar 对象
- source: 被复制的 schar 对象
- 返回: 0 成功, 1 失败

> 复制 source 到 dest 里。
> 它其实是 schar_copyb(dest, source->s, source->len);

```
int schar_catb(schar *dest, char *src, unsigned int n)
```

- dest: 目标 schar 对象
- src: 被追加的字符串
- n: 追加前 n 个字符
- 返回: 0 成功, 1 失败

> 追加 src 前 n 个字节到 dest 里，并以 \0 结束。
> 如果目标 dest 内存不够，dest 会自动扩展内存并复制。

```
int schar_cats(schar *dest, char *str)
```

- dest: 目标 schar 对象
- src: 被追加的字符串
- 返回: 0 成功, 1 失败

> 追加 src 字符串到 dest 里, 并以 \0 结束。
> 它其实是 schar_catb(dest, str, strlen(str))

```
int schar_cat(schar *dest, schar *source)
```

- dest: 目标 schar 对象
- source: 被追加的 schar 对象
- 返回: 0 成功, 1 失败

> 追加 source 到 dest 里。
> 它其实是 schar_catb(dest, source->s, source->len);

```
int schar_strpos(schar *dest, char *substr, int substr_len, unsigned int offset)
```

- dest: 被查找的源
- substr: 需要查找的子字符串
- substr_len: 子字符串的长度
- offset: 从 offset 的位置开始查找.
- 返回: 返回第一次出现的位置，未查到返回 SCHAR_NOTFOUND (-1)

> 找子字符串 str 在 dest 中第一次出现的位置。

```
int schar_stripos(schar *dest, char *substr, int substr_len, unsigned int offset)
```

> 同上面函数一样，此函数不区分大小写。

```
char *schar_strstr_alloc(schar *dest, char *substr, int substr_len, unsigned int offset, unsigned int before_substr)
```

- dest: 被查找的源
- substr: 需要查找的子字符串
- substr_len: 子字符串的长度
- offset: 从 offset 的位置开始查找.
- before_substr: 默认是 0, 为 1 时，返回的是 substr 第一次出现之前的字符串
- 返回: 返回查找到的字符串，该字符串必须手动 free，失败返回 NULL

> 查找子字符串 substr 在 dest 中第一次出现。 返回的结果需要手动 free()

```
char *schar_stristr_alloc(schar *dest, char *substr, int substr_len, unsigned int offset, unsigned int before_substr)
```

> 同上面的函数一样，此函数不区分大小写

```
void schar_strtolower(schar *x)
```

- x: 被转小写的 schar

> 转换字符串为小写, x 内字符被直接修改成小写。

```
void schar_strtoupper(schar *x
```

> 转换字符串为大写，x 内字符被直接修改成大写。
