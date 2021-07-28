# sio

封装了 io 与 socket io 操作

## 一. 使用方法

a. 引入文件

```c
#include "io/sio.h"
```

b. 写个 socket client

```c
#include <libgen.h>
#include <netinet/tcp.h>
#include <netdb.h>

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void main(int argc, char **argv)
{
    char *host = argv[1];
    char *port = argv[2];

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Connect --------------------------
    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0)
    {
        printf("> getaddrinfo: %s:%s fail:%s\n", host, port, gai_strerror(rv));
        return;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            printf("> create socket for arc fail[%s:%d]:%s\n", host, port, strerror(errno));
            continue;
        }

        // if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        if (sock_connect(sockfd, p->ai_addr, p->ai_addrlen, 3) != 0)
        {
            printf("> Connect to %s:%s fail:%s\n", host, port, strerror(errno));
            continue;
        }

        break;
    }
    if (p == NULL)
    {
        printf("> Connect to %s:%s fail:%s\n", host, port, strerror(errno));
        return;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
    printf("> Connect to %s:%s succ.\n", s, port);

    freeaddrinfo(servinfo);

    char buf[1024] = {0};
    for (;;)
    {
        int r = sock_read(sockfd, buf, sizeof(buf) - 1, 10);
        printf("r:%d buf:%s\n", r, buf);
    }
}
```

## 二. 函数说明

```
int sock_connect(int fd, const struct sockaddr *addr, socklen_t addrlen, unsigned int timeout)
```

- fd: socket fd
- addr: 由 addr 指定的地址
- addrlen: addr 的大小
- timeout: 连接超时, 单位:秒
- 返回: 0 成功, -1 失败, -2 超时

> 连接一个 socket

```
ssize_t sock_read(int sockfd, char *buf, unsigned int count, unsigned int timeout)
```

- sockfd: socket fd
- buf: 保存读取的字节数 buffer
- count: 尝试读取的字节数
- timeout: 读取超时时间, 单位:秒

> 尝试从 sockfd 读取 count 个字节到 buf 中

```
ssize_t sock_write(int sockfd, char *buf, unsigned int count, unsigned int timeout)
```

- sockfd: socket fd
- buf: 保存写入的字节数 buffer
- count: 尝试写入的字节数
- timeout: 写入超时时间, 单位:秒

> 写入 count 个 buf 字节到 sockfd 中

```
ssize_t safe_read(int fd, char *buf, size_t count, unsigned int timeout)
```

- fd: 读取的 fd
- buf: 保存读取的字节 buffer
- count: 尝试读取的字节数
- timeout: 读取的超时时间, 单位:秒
- 返回: -2 超时, -1 错误, 0 文件结束, 大于 0 为读取的字节数

> 封装系统函数 read, 带有 timeout
> 如果成功，则返回读取的字节数(0 表示文件结束), 如果出现错误，则返回-1，并适当设置 errno.

```
ssize_t safe_write(int fd, const char *buf, size_t count, unsigned int timeout)
```

- fd: 写入的 fd
- buf: 保存需要写入的字节 buffer
- count: 尝试写入的字节数
- timeout: 写入的超时时间, 单位:秒
- 返回: -2 超时, -1 错误, 大于 0 为读取的字节数

> 封装系统函数 read, 带有 timeout
> 如果成功，则返回写入的字节数, 如果出现错误，则返回-1，并适当设置 errno.

## SIO_SSL

### 做 ssl server 步骤:

1.  create socket, bind and listen it
2.  create ssl_ctx with cert and key: sock_ssl_ctx_new();

3.          accept a client fd
4.          create ssl for the client fd: ssl = sock_ssl_new_client();
5.          read and write with security socket: sock_ssl_read(), sock_ssl_write()
6.          close client:
            a. sock_ssl_free_client(ssl);
            b. close(client_fd);

7.  close listen fd: close(listen_fd);
8.  cleanup ssl: sock_ssl_ctx_free(ctx);

```c
    listenfd = socket_bind_listen(port);
    ctx = sock_ssl_ctx_new(crt, key);
    while(1) {
        // accept
        cfd = accept(listenfd, ...);
        ssl = sock_ssl_new_client(cfd);

        sock_ssl_read(ssl, buf, ...);
        sock_ssl_write(ssl, buf, ...);

        sock_ssl_free_client(ssl);
        close(cfd);
    }
    close(listenfd);
    sock_ssl_ctx_free(ctx);
```

### 制作证书和私钥

1. Generate the CA private key and certificate:

```bash
openssl genrsa -out ca_private.key 2048
openssl req -x509 -new -nodes -key ca_private.key -out ca_private.pem -days 365
```

2. Generate the private key and certificate to be signed

```bash
openssl genrsa -out serv_private.key 2048
openssl req -new -key serv_private.key -out serv_private.csr
```

3. Sign the server certificate using the CA certificate and private key:

```bash
openssl x509 -req -in serv_private.csr -CA ca_private.pem -CAkey ca_private.key -CAcreateserial -out serv_private.crt -days 365
```

### 做 ssl client 步骤:

1. create ssl_ctx with cert and key: ctx = sock_ssl_ctx_new();
2. connect to remote server: fd = s_connect();
3. with tls/ssl server handshake: ssl = sock_ssl_connect()
4. read and write with security socket: sock_ssl_read(), sock_ssl_write()
5. close socket and cleanup ssl:
   a. close(sockfd);
   b. sock_ssl_cleanup(ssl);
   c. sock_ssl_ctx_free(ctx);

### 例子：

- ssl_client:

```c
#include <netdb.h>

// 获取 sockaddr IPv4或IPv6
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)sa)->sin_addr);
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int s_connect(char *ip, char *service, char *err, size_t err_size)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN] = {0};
    // int pp = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(ip, service, &hints, &servinfo)) != 0)
    {
        snprintf(err, err_size, "getaddrinfo: %s", gai_strerror(rv));
        return -1;
    }

    // 循环找出全部的结果，并绑定(bind)到第一个能用的结果
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family,
                             p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            snprintf(err, err_size, "socket: %s", strerror(errno));
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            snprintf(err, err_size, "connect: %s", strerror(errno));
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        snprintf(err, err_size, "client: failed to connect");
        return -1;
    }

    inet_ntop(p->ai_family,
              get_in_addr((struct sockaddr *)&p->ai_addr),
              s, sizeof(s));

    printf("client: connecting to %s succ\n", s);

    freeaddrinfo(servinfo);

    return sockfd;
}

int sockfd = -1;
SSL_CTX *ctx = NULL;
SSL *ssl = NULL;

int main(int argc, char **argv)
{
    char *ip = argv[1];
    char *port = argv[2];
    char *crtfile = NULL;
    char *keyfile = NULL;

    if (argc < 2)
    {
        printf("Usage: %s <ip> <port|service name> [<crt> <key>]\n", argv[0]);
        return 1;
    }

    int ret = -1;
    char err[1024] = {0};
    size_t err_size = sizeof(err);

    ctx = sock_ssl_ctx_new(1, crtfile, keyfile, err, err_size);
    if (!ctx)
    {
        printf("Fail: %s\n", err);
        return 1;
    }

    sockfd = s_connect(ip, port, err, err_size);
    if (sockfd == -1)
    {
        printf("Fail: %s\n", err);
        sock_ssl_ctx_free(ctx);
        return 1;
    }
    if (_debug)
        printf("connect to %s:%s succ\n", ip, port);

    ssl = sock_ssl_connect(ctx, sockfd, err, err_size);
    if (!ssl)
    {
        printf("Fail: %s\n", err);
        close(sockfd);
        sock_ssl_ctx_free(ctx);
        return 1;
    }
    if (_debug)
        printf("ssl connect succ\n");

    ret = check_certs(ssl);
    if (ret == -2)
        printf("Server is no certificate\n");
    else if (ret == -1)
        printf("Server certificate check fail\n");
    else
        printf("Server certificat check succ\n");

    int i;
    fd_set rfds, afds;
    FD_ZERO(&afds);
    FD_SET(sockfd, &afds);

    char buf[1024] = {0};
    char sbuf[1024] = {0};
    int is_exit = 0;

    while (!is_exit)
    {
        rfds = afds;
        if (select(sockfd + 1, &rfds, NULL, NULL, NULL) < 0)
        {
            printf("select() fail: %s\n", strerror(errno));
            break;
        }

        for (i = 0; i < FD_SETSIZE; ++i)
        {
            if (FD_ISSET(i, &rfds))
            {
                printf("ready read\n");
                int n = sock_ssl_read(ssl, i, buf, sizeof(buf), 3, err, err_size);
                if (n < 0)
                {
                    printf("Fail: %s\n", err);
                    goto SFAIL;
                }
                buf[n] = '\0';
                printf("read: %s\n", buf);

                fgets(sbuf, sizeof(sbuf), stdin);

                n = sock_ssl_write(ssl, i, sbuf, strlen(sbuf), 3, err, err_size);
                printf("write: %d: %s", n, sbuf);

                if (strncasecmp(sbuf, "exit", 4) == 0)
                {
                    printf("i'll exit.\n");
                    is_exit = 1;
                    break;
                }

                sleep(1);
            }
        }
    }

    printf("Bye.\n");

    // if (sockfd != -1)
    close(sockfd);
    sock_ssl_cleanup(ssl);
    sock_ssl_ctx_free(ctx);
    return 0;

SFAIL:
    close(sockfd);
    sock_ssl_cleanup(ssl);
    sock_ssl_ctx_free(ctx);

    return 1;
}
```

- ssl_server.c:

```c
#include <netdb.h>

// 获取 sockaddr IPv4或IPv6
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)sa)->sin_addr);
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
// 获取 port
in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in *)sa)->sin_port);
    return (((struct sockaddr_in6 *)sa)->sin6_port);
}

#define BACKLOG 10 // 有多少个特定的连接队列 (pending connections queue)
/**
 * @brief Create, Bind and listen socket fd
 * @param service   The port for listen or service name (see services(5))
 * @param err       The error string
 * @param err_size  The memory size of err
 * @return -1:fail, the err is error string. Other is succ
 *
 * It return socket fd
 */
int sock_listener(char *service, char *err, size_t err_size)
{
    int sockfd = -1;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int on = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // 使用自己的IP

    if ((rv = getaddrinfo(NULL, service, &hints, &servinfo)) != 0)
    {
        snprintf(err, err_size, "getaddrinfo: %s", gai_strerror(rv));
        return -1;
    }

    // 循环找出全部的结果，并绑定(bind)到第一个能用的结果
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family,
                             p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            snprintf(err, err_size, "socket: %s", strerror(errno));
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1)
        {
            close(sockfd);
            snprintf(err, err_size, "setsockopt: %s", strerror(errno));
            return -1;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            snprintf(err, err_size, "bind: %s", strerror(errno));
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        snprintf(err, err_size, "server: failed to bind");
        return -1;
    }

    freeaddrinfo(servinfo); //  全部都用这个structure

    if (listen(sockfd, BACKLOG) == -1)
    {
        close(sockfd);
        snprintf(err, err_size, "listen: %s", strerror(errno));
        return -1;
    }

    return sockfd;
}

/**
 * @brief Accept a connection on a socket
 * @param listenfd      Pointer to listen port fd
 * @param rip           Client IP string
 * @param rip_size      The memory size of rip
 * @param rport         Client Port string
 * @param rport_size    The memory size of rport
 * @param err           The error string
 * @param err_size      THe memory size of err
 * @return return client fd, it return -1 when error, check err
 *
 * On success, these system calls return a nonnegative integer that
 * is a descriptor for the accepted socket.
 * On  error, -1 is returned, check err.
 */
int sock_accept(int listenfd, char *rip, size_t rip_size, char *rport, size_t rport_size, char *err, size_t err_size)
{
    int new_fd;
    struct sockaddr_storage their_addr; // 连接者的地址信息
    socklen_t sin_size = sizeof(their_addr);
    char s[INET6_ADDRSTRLEN] = {0};

    new_fd = accept(listenfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1)
    {
        snprintf(err, err_size, "accept: %s", strerror(errno));
        return -1;
    }

    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof(s));

    if (rip_size > 0 && rip != NULL)
        snprintf(rip, rip_size, "%s", s);
    if (rport_size > 0 && rport != NULL)
        snprintf(rport, rport_size, "%d", ntohs(get_in_port((struct sockaddr *)&their_addr)));

    return new_fd;
}

#include <signal.h>

void sig_catch(int sig, void (*f)())
{
    struct sigaction sa;
    sa.sa_handler = f;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(sig, &sa, (struct sigaction *)0);
}

typedef struct client
{
    int fd;
    SSL *ssl;
    char ip[1024];
    char port[128];
} client;

int main(int argc, char **argv)
{
    if (argc != 4 && argc != 5)
    {
        printf("Usage: %s <port|service name> <crt> <key>\n", argv[0]);
        printf("two-way auth: %s <port> <server crt> <server key> <ca crt>\n", argv[0]);
        return 1;
    }

    char *port = argv[1];
    char *crtfile = argv[2];
    char *keyfile = argv[3];
    char *cafile = NULL;

    if (argc == 5)
        cafile = argv[4];

    sig_catch(SIGPIPE, SIG_IGN);

    int ret = -1;
    char err[1024] = {0};
    size_t err_size = sizeof(err);

    // Create socket and listen it
    int sockfd, clientfd;
    sockfd = sock_listener(port, err, err_size);
    if (sockfd == -1)
    {
        printf("%s\n", err);
        return 1;
    }

    // Init SSL library
    SSL_CTX *ctx = NULL;
    // ssl = sock_ssl_new(crtfile, keyfile, err, err_size);
    ctx = sock_ssl_ctx_new(0, crtfile, keyfile, err, err_size);
    if (!ctx)
    {
        printf("%s\n", err);
        return 1;
    }

    if (cafile)
    {
        // 设置双向认证
        if (sock_ssl_use_2way_auth(ctx, cafile, err, err_size) == -1)
        {
            printf("Fail: %s\n", err);
            return 1;
        }
    }

    int max_client_num = 10;
    int client_len = 0;
    struct client **client_list = (struct client **)calloc(max_client_num,
                                                           sizeof(struct client *));
    if (client_list == NULL)
    {
        printf("out of memory\n");
        return 1;
    }

    int is_exit = 0;
    int i;
    char buf[1024] = {0};
    char sbuf[1024] = {0};
    int max_fd = sockfd;
    fd_set afds, rfds;
    FD_ZERO(&afds);
    FD_SET(sockfd, &afds);

    while (!is_exit)
    {
        rfds = afds;
        // select the ready descriptor
        if (select(max_fd + 1, &rfds, NULL, NULL, NULL) < 0)
        {
            printf("select() fail: %s\n", strerror(errno));
            break;
        }

        for (i = 0; i < FD_SETSIZE; ++i)
        {
            if (FD_ISSET(i, &rfds))
            {
                if (i == sockfd)
                {
                    // Accept
                    struct client *c = calloc(1, sizeof(struct client));
                    if (c == NULL)
                    {
                        continue;
                    }

                    c->fd = sock_accept(sockfd,
                                        c->ip, sizeof(c->ip),
                                        c->port, sizeof(c->port),
                                        err, err_size);
                    if (c->fd == -1)
                    {
                        printf("%s\n", err);
                        free(c);
                        continue;
                    }
                    if (_debug)
                        printf("Client %s:%s connected\n", c->ip, c->port);

                    c->ssl = sock_ssl_new_client(ctx, c->fd, err, err_size);
                    if (!c->ssl)
                    {
                        printf("Fail: %s\n", err);
                        close(c->fd);
                        free(c);
                        continue;
                    }
                    if (_debug)
                        printf("set client ssl succ\n");

                    *(client_list + client_len++) = c;

                    // add client fd to fd_set
                    FD_SET(c->fd, &afds);
                    if (c->fd > max_fd)
                        max_fd = c->fd;

                    snprintf(sbuf, sizeof(sbuf), "Greeting...\n");
                    ret = sock_ssl_write(c->ssl, c->fd, sbuf, strlen(sbuf), 3, err, err_size);
                    printf("ssl write %d:%s", ret, sbuf);

                    int ret = check_certs(c->ssl);
                    if (ret == -2)
                        printf("Client is no certificate\n");
                    else if (ret == -1)
                        printf("Client certificate check fail\n");
                    else
                        printf("Client certificat check succ\n");
                }
                else
                {
                    // int cfd = i;

                    int ci = 0;
                    for (ci = 0; ci < client_len; ci++)
                    {
                        if (*(client_list + ci) != NULL && (*(client_list + ci))->fd == i)
                            break;
                    }
                    struct client *c = *(client_list + ci);

                    ret = sock_ssl_read(c->ssl, c->fd, buf, sizeof(buf), 3, err, err_size);
                    if (ret == -2)
                        printf("ssl read timeout\n");
                    else if (ret == 0 || ret == -1)
                    {
                        if (ret == 0)
                            printf("close client(%s:%s)\n", c->ip, c->port);
                        else
                            printf("ssl read fail ret(%d): %s\n", ret, err);

                        // sock_ssl_cleanup(c->ssl);
                        sock_ssl_free_client(c->ssl);
                        close(c->fd);
                        FD_CLR(c->fd, &afds);

                        free(*(client_list + ci));
                        *(client_list + ci) = NULL;

                        continue;
                    }
                    else
                    {
                        buf[ret] = '\0';
                        printf("ssl read(%s:%s) %d: %s", c->ip, c->port, ret, buf);

                        snprintf(sbuf, sizeof(sbuf), "Get OK: %s", buf);
                        ret = sock_ssl_write(c->ssl, c->fd, sbuf, strlen(sbuf), 3, err, err_size);
                        if (ret == -2)
                            printf("ssl write timeout\n");
                        else if (ret == -1)
                            printf("ssl write fail: %s\n", err);
                        else
                            printf("ssl write(%s:%s) %d: %s", c->ip, c->port, ret, sbuf);

                        if (strncasecmp(buf, "master exit", 11) == 0)
                        {
                            // master shutdown
                            is_exit = 1;
                            break;
                        }
                    }
                }
            }
        }
    }

    close(sockfd);
    sock_ssl_ctx_free(ctx);

    for (i = 0; i < client_len; i++)
    {
        if (*(client_list + i))
            free(*(client_list + i));
    }
    free(client_list);

    printf("Bye..\n");
    return 0;
}
```
