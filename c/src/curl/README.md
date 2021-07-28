# scurl

封装了 libcurl 常用操作

## 使用方法

a. 引入头文件

```c
#include "curl/scurl.h"
```

b. 添加自定义文件头，如果不需要则设置 NULL

```c
struct curl_slist *headers = NULL;

// 如果没有自定义头，忽略下面的
headers = scurl_header_add(headers, "X-User: kyosold);
headers = scurl_header_add(headers, "X-UID: 11232121");
```

- 设置的自定义头，在执行操作后会自动释放内存，不需要手动释放。

c. Get 提交操作

```c
char *url = "http://www.baidu.com/index.php?id=1000&name=sj";
int conn_timeout = 1;
int exec_timeout = 3

struct scurl_resp_s scurl_resp;
int ret = scurl_get(url, conn_timeout, exec_timeout,
                    NULL, NULL, &scurl_resp);
printf("ret:%d code:%r\n", ret, scurl_resp.response_code);
printf("Http Header:\n%s\n", scurl_resp.header.str);
printf("Http Body:\n%s\n", scurl_resp.body.str);

if (scurl_resp.header.str)
    free(scurl_resp.header.str);
if (scurl_resp.body.str)
    free(scurl_resp.body.str);
```

d. http response 结构体

```c
typedef struct scurl_resp_s
{
    unsigned long response_code;
    struct pstr_s header;
    struct pstr_s body;
} scurl_resp_t;
```

- response_code: http response code
- header: http response header
- body: http response body

### 编译

需要指定 libcurl 库, 例如:

```bash
-I/usr/include/ -L/usr/lib64/ -lcurl
```

## 函数说明

```
int scurl_get(char *url, struct curl_slist *headers,
            unsigned int connect_timeout, unsigned int timeout,
            char *save_cookie_fs, char *send_cookie_fs,
            struct scurl_resp_s *scurl_resp)
```

- url: 指定提交 get 的地址，包括: ?A=a&B=b
- headers: 指定自定义头，如果不需要则设置为 NULL
- connect_timeou: 连接超时, 单位:秒
- timeout: 执行提交的超时, 单位:秒
- save_cookie_fs: 指定保存 cookie 信息的文件，不需要则设置 NULL
- send_cookie_fs: 指定提交给 url 的本地 cookie 文件，不需要则设置 NULL
- scurl_resp: 获取 http response 的信息
- 返回: 0 成功，1 失败

> 执行 http get 提交
> scurl_resp 需要手动释放内存
