# siconv

封装 libiconv 库的操作

## 使用方法

a. 引入头文件

```c
#include "iconv/siconv.h"
```

b. 字符串转换

```c
char *src = "abcčde";
size_t dst_len = 0;
char err[1024] = {0};

char *dst = s_iconv_string_alloc(src, strlen(src),
                            &dst_len, "UTF-8", "CP1250",
                            err, sizeof(err));
if (dst == NULL) {
    printf("convert fail:%s\n", err);
    return 1;
}
printf("convert result:%d (%s)\n", dst_len, dst);
free(dst);
```

## 函数说明

```
char *s_iconv_string_alloc(const char *in_p, size_t in_len,
                           size_t *out_len,
                           const char *out_charset, const char *in_charset,
                           char *err_str, size_t err_str_size)
```

- in_p: 需要被转换的字符串
- in_len: 被转换的字符串的长度
- out_len: 转换后的字符串的长度
- out_charset: 目的字符集
- in_charset: 源字符集
- err_str: 当有错误时，保存错误信息
- err_str_size: 保存错误信息 buffer 的空间
- 返回: 返回被转换后的字符串, 失败返回 NULL

> 返回的结果如果有值，需要手动 free()
