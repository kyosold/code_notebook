<!--
 * @Author: songjian <kyosold@qq.com>
 * @Date: 2022-08-12 14:53:15
 * @LastEditors: kyosold kyosold@qq.com
 * @LastEditTime: 2022-08-12 15:04:55
 * @FilePath: /socket/README.md
 * @Description: 
 * 
 * Copyright (c) 2022 by kyosold kyosold@qq.com, All Rights Reserved. 
-->
# socket

封装了 socket io 操作

> 具体代码可以看: example.c 或 example_ssl.c


## 1. 普通的socket使用
---

### - Client 模式:
1). 引入头文件
```c
#include "socket_io.h"
```
2). 连接到服务器
```c
int socket_fd = socket_connect(ip, port, timeout);
if (socket_fd == -1) {
    printf("socket_connect to %s:%s fail: %s\n", ip, port, get_socket_error_info());
    return 1;
}
```
3). 读、写
```c
int process_child(int conn_fd, int timeout)
{
    char buf[1024] = {0};
    int n = 0;

    // greeting
    snprintf(buf, sizeof(buf), "220 greeting\r\n");
    n = safe_write(conn_fd, buf, strlen(buf), timeout);
    printf("write to remote: [%d]:%s\n", n, buf);

    for (;;) {
        n = socket_read(conn_fd, buf, sizeof(buf), timeout);
        if (n == -1) {
            if (errno == ETIMEDOUT) {
                printf("read from remote timeout:%d\n", timeout);
                continue;		// 对于超时的控制，自己可修改
            }
            printf("socket_read fail: %s\n", get_socket_error_info());
            return -1;
        } else if (n == 0) {
            printf("remote disconnected\n");
            return 0;
        }
        buf[n] = '\0';
        printf("read from remote: [%d]:%s\n", n, buf);

        snprintf(buf, sizeof(buf), "250 OK\r\n");
        n = socket_write(conn_fd, buf, strlen(buf), timeout);
        printf("write to remote: [%d]:%s\n", n, buf);
    }
    return 0;
}
```
4). 断开连接
```c
close(socket_fd);
```

### - Server 模式:
1). 引入头文件
```c
#include "socket_io.h"
```
2). Listen 端口
```c
int listen_fd = socket_listen(port, backlog);
if (listen_fd == -1) {
    printf("socket_listen fail: %s\n", get_socket_error_info());
    return 1;
}
```
3). Accept client
```c
int conn_fd;
struct sockaddr_storage client_addr;
socklen_t sin_size = sizeof(client_addr);
char *client_ip;
while (1) {
    conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &sin_size);
    if (conn_fd == -1)
        continue;

    char *cip = get_remote_host(&client_addr);
    char *cport = get_remote_port(&client_addr);

    char client_ip[INET6_ADDRSTRLEN] = {0};
    char client_port[128] = {0};
    snprintf(client_ip, sizeof(client_ip), "%s", cip);
    snprintf(client_port, sizeof(client_port), "%s", cport);

    printf("Client %s:%s connected.\n", client_ip, client_port);

    pid_t child_pid = fork();
    if (child_pid == 0) {
        close(listen_fd);

        /********************************************
         * 3. read/write with client
         *******************************************/
        int n = process_child(conn_fd, timeout);
        printf("child exit(%d)\n", n);
    }
    close(conn_fd);
}
```
4). Read/Write
```c
int process_child(int conn_fd, int timeout)
{
    char buf[1024] = {0};
    int n = 0;

    // greeting
    snprintf(buf, sizeof(buf), "220 greeting\r\n");
    n = safe_write(conn_fd, buf, strlen(buf), timeout);
    printf("write to remote: [%d]:%s\n", n, buf);

    for (;;) {
        n = socket_read(conn_fd, buf, sizeof(buf), timeout);
        if (n == -1) {
            if (errno == ETIMEDOUT) {
                printf("read from remote timeout:%d\n", timeout);
                continue;		// 对于超时的控制，自己可修改
            }
            printf("socket_read fail: %s\n", get_socket_error_info());
            return -1;
        } else if (n == 0) {
            printf("remote disconnected\n");
            return 0;
        }
        buf[n] = '\0';
        printf("read from remote: [%d]:%s\n", n, buf);

        snprintf(buf, sizeof(buf), "250 OK\r\n");
        n = socket_write(conn_fd, buf, strlen(buf), timeout);
        printf("write to remote: [%d]:%s\n", n, buf);
    }
    return 0;
}
```
5). 断开连接
```c
close(socket_fd);
```


## 2. 加密SSL的socket使用
---

### - Client 模式:

**基本框架:**
1. 使用普通方法connect到server
2. 对fd做加密，生成ssl
3. 使用ssl进行加密读写
4. 关闭ssl
5. 关闭fd


1). 引入头文件
```c
#include "socket_ssl_io.h"
```
2). connect to server
```c
int socket_fd = socket_connect(ip, port, timeout);
if (socket_fd == -1) {
    printf("socket_connect to %s:%s fail: %s\n", ip, port, get_socket_error_info());
    return 1;
}
```
3). 由 socket fd 生成 ssl
```c
// 初始化ssl ctx
ctx = ssl_socket_new_ctx(1);
if (ctx == NULL) {
    printf("ssl_socket_new_ctx fail: %s\n", get_socket_error_info());
    return 1;
}
// 设置证书验证方式， 如果不需要验证可以不写，或使用 SSL_SOCKET_VERIFY_NONE
ssl_socket_set_verify_certificates(ctx, SSL_SOCKET_VERIFY_ALL, depth);
// 由 socket fd 生成 ssl
ssl = ssl_socket_connect(ctx, socket_fd, timeout);
if (ssl == NULL) {
    ssl_socket_free_ctx(ctx);
    printf("ssl_socket_free_ctx fail: %s\n", get_socket_error_info());
    return 1;
}
// show certificates
show_certs(ssl);	// 不需要可以不写
```
4). Read/Write
```c
// read
char buf[1024] = {0};
int n = ssl_socket_read(ssl, socket_fd, buf, sizeof(buf), timeout);
printf("read[%d]: %s\n", n, buf);
if (n <= 0)
{
		if (n == 0) {
				printf("remote disconnect\n");	// 对方关闭连接，自己决定怎么处理
		} else if (n == -1) {
				if (errno == ETIMEDOUT) {
						printf("ssl read timeout:%d\n", timeout);	// 读超时，自己决定怎么处理
				}
		}
    printf("ssl_socket_read fail: %s\n", get_socket_error_info());

    ssl_socket_free_ctx(ctx);
    ssl_socket_cleanup(ssl);

    close(socket_fd);
    return 1;
}
// 2.4 write
snprintf(buf, sizeof(buf), "HELO gpkeys.com\r\n");
n = ssl_socket_write(ssl, socket_fd, buf, strlen(buf), timeout);
printf("write[%d]: %s\n", n, buf);
```

### - Server 模式:

**基本框架:**
1. listen 端口
2. accept client
3. 对 client fd 做加密，生成ssl
4. 使用 ssl 进行加密读写
5. 关闭 ssl
6. 关闭 client fd


1). 引入头文件
```c
#include "socket_ssl_io.h"
```
2). listen local port
```c
int listen_fd = socket_listen(port, backlog);
if (listen_fd == -1) {
    printf("socket_listen fail: %s\n", get_socket_error_info());
    return 1;
}
```
3). accept client
```c
int conn_fd;
struct sockaddr_storage client_addr;
socklen_t sin_size = sizeof(client_addr);
char *client_ip;

while (1) {
    conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &sin_size);
    if (conn_fd == -1)
        continue;

    char *cip = get_remote_host(&client_addr);
    char *cport = get_remote_port(&client_addr);

    char client_ip[INET6_ADDRSTRLEN] = {0};
    char client_port[128] = {0};
    snprintf(client_ip, sizeof(client_ip), "%s", cip);
    snprintf(client_port, sizeof(client_port), "%s", cport);

    printf("Client %s:%s connected.\n", client_ip, client_port);

    pid_t child_pid = fork();
    if (child_pid == 0) {
        close(listen_fd);

        int n = process_child(conn_fd, cert_file, key_file);
        printf("child exit(%d)\n", n);
    }
    close(conn_fd);
}
```
4). 对 conn_fd 进行加密，并进行读写
```c
int process_child(int conn_fd, char *cert_file, char *key_file)
{
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;

    /********************************************
     * 3. 对 client socket fd 做加密
     *******************************************/
    // 3.1 初始化ssl ctx
    ctx = ssl_socket_new_ctx(0);
    if (ctx == NULL)
    {
        close(conn_fd);
        printf("ssl_socket_new_ctx cert_file(%s) key_file(%s) fail: %s\n", cert_file, key_file, get_socket_error_info());
        return -1;
    }
    load_certificates(ctx, cert_file, key_file, NULL);

    // 3.2 使用accept生成ssl
    ssl = ssl_socket_accept(ctx, conn_fd, 10);
    if (ssl == NULL)
    {
        ssl_socket_free_ctx(ctx);
        close(conn_fd);
        printf("ssl_socket_accept fail: %s\n", get_socket_error_info());
        return -1;
    }
    // 3.4 show certificates
    show_certs(ssl);

    // 3.3 process
    int n = 0;
    char buf[1024] = {0};
    int buf_size = sizeof(buf);

    snprintf(buf, buf_size, "greeting example\r\n");
    n = ssl_socket_write(ssl, conn_fd, buf, strlen(buf), 3);
    printf("write to client: [%d]%s", n, buf);

    for (;;)
    {
        n = ssl_socket_read(ssl, conn_fd, buf, buf_size, 3);
        if (n == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            if (errno == ETIMEDOUT)
            {
                printf("client read timeout, we exit.\n");
                break;
            }
            printf("ssl_socket_read read fail: %s\n", get_socket_error_info());
            break;
        }
        else if (n == 0)
        {
            printf("client disconnect\n");
            break;
            ;
        }
        buf[n] = '\0';
        printf("read from client: [%d]%s", n, buf);

        snprintf(buf, buf_size, "250 ok\r\n");
        n = ssl_socket_write(ssl, conn_fd, buf, strlen(buf), 3);
        printf("write to client: [%d]%s", n, buf);
    }

    ssl_socket_free_ctx(ctx);
    ssl_socket_cleanup(ssl);
    close(conn_fd);

    return n;
}
```