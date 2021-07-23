# 编码

## 一. 通常用法

a. Base64 编/解码

```c
#include "code/base64.h"

char *str = "123qwe";
int outlen = 0;
char *out = s_base64_encode_alloc(str, strlen(str), &outlen);
printf("b64 encode len:%d value:%s\n", outlen, out);

char *out2 = s_base64_decode_alloc(out, outlen, &outlen);
printf("b64 decode len:%d value:%s\n", outlen, out2);

free(out);
free(out2);
```

b. Quoted_printable 编/解码

```c
#include "code/quote_print.h"

char *str = "123qwe";
size_t outlen = 0;
char *out = s_quoted_printable_encode_alloc(str, strlen(str), &outlen);
printf("qp encode len:%d value:%s\n", outlen, out);

char *out2 = s_quoted_printable_decode_alloc(out, outlen, &outlen);
printf("qp decode len:%d value:%s\n", outlen, out2);

free(out);
free(out2);
```

c. MD5 编码

```c
#include "code/md5.h"

char *str = "123qwe";
char res[64] = {0};
s_md5(str, strlen(str), 0, res, sizeof(res));
```

d. SHA1 编码

```c
#include "code/sha1.h"

char *str = "123qwe";
char res[64] = {0};
s_sha1(str, strlen(str), 0, res, sizeof(res));

char *f = "./abc.txt";
s_sha1_file(f, 0, res, sizeof(res));
```

e. CRC32 编码

```c
#include "code/crc32.h"

char *str = "123qwe";
char res[64] = {0};
s_crc32(str, strlen(str), res, sizeof(res));
```

f. Uniqid

```c
#include "code/uniqid.h"

char uid[64] = {0};
s_uniqid("", uid, sizeof(uid));
```

## 二. 函数说明

#### Base64

```
unsigned char *s_base64_encode_alloc(const unsigned char *str, int str_len, int *ret_length);
```

- str: 需要被编码的字符串
- str_len: 字符串的长度
- ret_length: 编码后的字符串的长度
- 返回: 编码后的字符串，需要手动 free 释放内存

> base64 编码字符串，结果需要手动 free

```
unsigned char *s_base64_decode_alloc(const unsigned char *str, int str_len, int *ret_length);
```

- str: 需要被解码的字符串
- str_len: 字符串的长度
- ret_length: 解码后的字符串长度
- 返回: 解码后的字符串，需要手动 free 释放内存

> base64 解码字符串，结果需要手动 free

#### Quoted_Printable

```
unsigned char *s_quoted_printable_encode_alloc(const char *str, size_t str_len, size_t *ret_length);
```

- str: 需要被解码的字符串
- str_len: 字符串的长度
- ret_length: 解码后的字符串长度
- 返回: 解码后的字符串，需要手动 free 释放内存

> Quoted_printable 编码字符串，结果需要手动 free

```
unsigned char *s_quoted_printable_decode_alloc(const char *str, size_t str_len, size_t *ret_length);
```

- str: 需要被解码的字符串
- str_len: 字符串的长度
- ret_length: 解码后的字符串长度
- 返回: 解码后的字符串，需要手动 free 释放内存

> Quoted_printable 解码字符串，结果需要手动 free

#### MD5

```
void s_md5(const char *str, size_t str_len, int raw_output, char *result, size_t result_size);
```

- str: 需要被编码的字符串
- str_len: 字符串的长度
- raw_output: 默认设 0，如果设 1，则以长度为 16 的原始二进制格式返回
- result: 存储编码后的结果
- result_size: result 的内存空间，普通大于 33

> 计算指定字符串的 md5 值

```
int s_md5_file(const char *filename, int raw_output, char *result, size_t result_size);
```

- filename: 需要被编码的文件名，包含路径
- raw_output: 默认设 0，如果设 1，则以长度为 16 的原始二进制格式返回
- result: 存储编码后的结果
- result_size: result 的内存空间，普通大于 33
- 返回: 0 成功, 1 失败

> 计算指定文件的 md5 值

#### SHA1

```
void s_sha1(const char *str, size_t str_len, int raw_output, char *result, size_t result_size);
```

- str: 需要被编码的字符串
- str_len: 字符串的长度
- raw_output: 默认设 0，如果设 1，则以长度为 20 的原始二进制格式返回
- result: 存储编码后的结果
- result_size: result 的内存空间，普通大于 40

> 计算指定字符串的 sha1 值

```
int s_sha1_file(const char *filename, int raw_output, char *result, size_t result_size);
```

- str: 需要被编码的文件
- raw_output: 默认设 0，如果设 1，则以长度为 20 的原始二进制格式返回
- result: 存储编码后的结果
- result_size: result 的内存空间，普通大于 40

> 计算指定文件的 sha1 值

#### CRC32

```
void s_crc32(const char *str, size_t str_len, char *result, size_t result_size);
```

- str: 需要被编码的字符串
- str_len: 字符串的长度
- result: 存储编码后的结果
- result_size: result 的内存空间，普通大于 32

> 计算指定字符串的 crc32 值

#### Uniqid

```
void s_uniqid(char *prefix, char *uid, size_t uid_size);
```

- prefix: 前缀，如果为空，则返回的字符串长度为 13 个字符
- uid: 保存以字符串形式返回基于时间戳的唯一标识符
- uid_size: uid 的内存空间

> 生成基于时间戳的唯一标识符的字符串
