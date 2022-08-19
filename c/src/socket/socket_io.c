/*
 * @Author: songjian <kyosold@qq.com>
 * @Date: 2022-08-11 11:29:24
 * @LastEditors: kyosold kyosold@qq.com
 * @LastEditTime: 2022-08-19 08:06:59
 * @FilePath: /socket/socket_io.c
 * @Description:
 *  Build:
 *      gcc -g -o socket_io_test socket_io.c -DTEST
 *  Exec:
 *      ./socket_io_test smtp.qq.com 25
 *
 * Copyright (c) 2022 by kyosold kyosold@qq.com, All Rights Reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include "socket_io.h"

#define MAX_DATA_SIZE 4096 // 4K

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

typedef long datetime_sec;

static char socket_err[1024] = {0};
char *get_socket_error_info()
{
    if (*socket_err)
        return socket_err;
    return "";
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)sa)->sin_addr);
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
int get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in *)sa)->sin_port);
    return (((struct sockaddr_in6 *)sa)->sin6_port);
}

static char remote_host[INET6_ADDRSTRLEN] = {0};
static char remote_port[128] = {0};
char *get_remote_host(struct sockaddr_storage *addr)
{
    inet_ntop(addr->ss_family,
              get_in_addr((struct sockaddr *)&addr),
              remote_host, sizeof(remote_host));
    return remote_host;
}
char *get_remote_port(struct sockaddr_storage *addr)
{
    snprintf(remote_port, sizeof(remote_port), "%d",
             ntohs(get_in_port((struct sockaddr *)addr)));
    return remote_port;
}

static char bind_host[INET6_ADDRSTRLEN] = {0};
char *get_bind_host(struct addrinfo *p)
{
    inet_ntop(p->ai_family,
              get_in_addr((struct sockaddr *)&p->ai_addr),
              bind_host,
              sizeof(bind_host));
    return bind_host;
}

static int connect_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeout);

int ndelay_on(int fd)
{
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

int ndelay_off(int fd)
{
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
}

/**
 * @description: 连接到指定的服务
 * @param {char} *ip            服务的IP
 * @param {char} *service       服务的端口或服务名(smtp,pop3)
 * @param {int} timeout         连接超时时间，单位:秒
 * @return {*}  返回socket fd，失败返回-1
 */
int socket_connect(char *ip, char *service, int timeout)
{
    int socket_fd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN] = {0};

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(ip, service, &hints, &servinfo)) != 0)
    {
        snprintf(socket_err, sizeof(socket_err), "getaddrinfo: %s", gai_strerror(rv));
        return -1;
    }

    // 循环找出全部的结果，并绑定(bind)到第一个能用的结果
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_fd = socket(p->ai_family,
                                p->ai_socktype,
                                p->ai_protocol)) == -1)
        {
            snprintf(socket_err, sizeof(socket_err), "create socket fail: %s", strerror(errno));
            continue;
        }
        if (connect_timeout(socket_fd, p->ai_addr, p->ai_addrlen, timeout) == -1)
        {
            close(socket_fd);
            snprintf(socket_err, sizeof(socket_err), "connect to %s:%s fail: %s", ip, service, strerror(errno));
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        snprintf(socket_err, sizeof(socket_err), "connect to %s:%s fail", ip, service);
        return -1;
    }

    inet_ntop(p->ai_family,
              get_in_addr((struct sockaddr *)&p->ai_addr),
              s, sizeof(s));
    // printf("connect to %s succ\n", s);

    freeaddrinfo(servinfo);

    return socket_fd;
}

/* 成功返回0，-1失败，检查 errno */
static int connect_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeout)
{
    char ch;
    socklen_t len;
    fd_set wfds;
    struct timeval tv;

    if (ndelay_on(sockfd) == -1)
        return -1;

    if (connect(sockfd, addr, addrlen) == 0)
    {
        ndelay_off(sockfd); // 连接成功
        return 0;
    }

    if ((errno != EINPROGRESS) && (errno != EWOULDBLOCK))
    {
        ndelay_off(sockfd); // 连接失败
        return -1;
    }

    FD_ZERO(&wfds);
    FD_SET(sockfd, &wfds);
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    if (select(sockfd + 1, NULL, &wfds, NULL, &tv) == -1)
    {
        ndelay_off(sockfd); // 连接失败
        return -1;
    }
    if (FD_ISSET(sockfd, &wfds))
    {
        if (getpeername(sockfd, (struct sockaddr *)addr, &addrlen) == -1)
        {
            read(sockfd, &ch, 1);
            return -1;
        }
        ndelay_off(sockfd); // 连接成功
        return 0;
    }

    errno = ETIMEDOUT;
    return -1;
}

/**
 * @description: listen到指定的端口
 * @param {char} *service   端口或服务名(smtp, pop3)
 * @param {int} backlog     待连接最大队列的数量
 * @return {*}  返回listen fd， 失败返回-1
 */
int socket_listen(char *service, int backlog)
{
    char bind_host[NI_MAXHOST];
    char bind_port[NI_MAXSERV];
    int rv = -1;
    int on = 1;

    int listen_fd, conn_fd, n;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage client_addr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, service, &hints, &servinfo)) != 0)
    {
        snprintf(socket_err, sizeof(socket_err), "getaddrinfo: %s", gai_strerror(rv));
        return -1;
    }

    // 循环找出全部的结果，并绑定(bind)到第一个能用的结果
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((rv = getnameinfo(p->ai_addr, p->ai_addrlen,
                              bind_host, sizeof(bind_host),
                              bind_port, sizeof(bind_port),
                              NI_NUMERICHOST | NI_NUMERICSERV)) != 0)
        {
            snprintf(socket_err, sizeof(socket_err), "getnameinfo fail: %s", gai_strerror(rv));
            continue;
        }

        if ((listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            snprintf(socket_err, sizeof(socket_err), "create socket fail: %s", strerror(errno));
            continue;
        }

        if (ndelay_on(listen_fd) == -1)
        {
            close(listen_fd);
            snprintf(socket_err, sizeof(socket_err), "set fd to nonblock fail: %s", strerror(errno));
            continue;
        }

        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
        {
            close(listen_fd);

            snprintf(socket_err, sizeof(socket_err), "setsockopt SO_REUSEADDR fail: %s", strerror(errno));
            continue;
        }

        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(listen_fd);
            snprintf(socket_err, sizeof(socket_err), "bind socket fail: %s", strerror(errno));
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        snprintf(socket_err, sizeof(socket_err), "bind to :%s fail", service);
        return -1;
    }

    freeaddrinfo(servinfo);

    if (listen(listen_fd, backlog) == -1)
    {
        snprintf(socket_err, sizeof(socket_err), "listen backlog(%d) fail: %s", backlog, strerror(errno));
        close(listen_fd);
        return -1;
    }

    printf("listen: %s:%s success\n", bind_host, bind_port);
    ndelay_off(listen_fd);

    return listen_fd;
}

/**
 * @description: 从指定的socket中读取数据
 * @param {int} socket_fd       指定需要读取的socket
 * @param {char} *buf           保存读取的数据
 * @param {size_t} buf_size     buf的空间大小
 * @param {int} timeout         读取超时时间，单位:秒
 * @return {*}  >0:读取的数量, 0:对方关闭连接, -1:读取失败，检查errno
 */
ssize_t socket_read(int socket_fd, char *buf, size_t buf_size, int timeout)
{
    ndelay_on(socket_fd);
    int r = 0;
    for (;;)
    {
        r = safe_read(socket_fd, buf, buf_size, timeout);
        if (r < 0)
        {
            if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
                continue;
        }
        break;
    }
    ndelay_off(socket_fd);
    return r;
}
/**
 * @description: 写数据到指定的socket
 * @param {int} socket_fd   写入数据的socket
 * @param {char} *buf       写入的数据buffer
 * @param {size_t} buf_len  写入数据的长度
 * @param {int} timeout     写入超时时间, 单位:秒
 * @return {*}  >0:写入的数量, 0:对方关闭连接, -1:写入失败，检查errno
 */
ssize_t socket_write(int socket_fd, char *buf, size_t buf_len, int timeout)
{
    ndelay_on(socket_fd);
    int r = 0;
    for (;;)
    {
        r = safe_write(socket_fd, buf, buf_len, timeout);
        if (r <= 0)
        {
            if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
                continue;
        }
        break;
    }
    ndelay_off(socket_fd);
    return r;
}

/**
 * @description: Attempts to read buf_size bytes from fd into the buf
 * @param {int} fd                  Read from fd
 * @param {char} *buf               Store to this buf
 * @param {size_t} buf_size         The memory size of bytes read from fd
 * @param {unsigned int} timeout    Read timeout. Seconds
 * @return {*}  >0:success read number of bytes., 0:end of file, -1:fail (check errno)
 */
/* 成功返回读取的长度，-1失败，检查 errno */
ssize_t safe_read(int fd, char *buf, size_t buf_size, unsigned int timeout)
{
    fd_set rfds;
    struct timeval tv;

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    if (select(fd + 1, &rfds, NULL, NULL, &tv) == -1)
        return -1;

    if (FD_ISSET(fd, &rfds))
        return read(fd, buf, buf_size);

    errno = ETIMEDOUT;
    return -1; // 失败
}
/**
 * @description: Attempts to write buf_len bytes of the buf to fd
 * @param {int} fd                  Write to fd
 * @param {char} *buf               Send data from this buf
 * @param {size_t} buf_len          The number bytes of buf
 * @param {unsigned int} timeout    Write timeout
 * @return {*}  >0:success write number of bytes., 0:end of file, -1:fail (check errno)
 */
/* 成功返回写入的长度，-1失败，检查 errno */
ssize_t safe_write(int fd, char *buf, size_t buf_len, unsigned int timeout)
{
    fd_set wfds;
    struct timeval tv;

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);

    if (select(fd + 1, NULL, &wfds, NULL, &tv) == -1)
        return -1;

    if (FD_ISSET(fd, &wfds))
        return write(fd, buf, buf_len);

    errno = ETIMEDOUT;
    return -1;
}

/***********************************************************
 * Socket SSL 相关操作
 **********************************************************/
#ifdef ssl_enable
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

static char ssl_err_buf[1024] = {0};
char *get_error_info()
{
    unsigned long r = ERR_get_error();
    if (!r)
        return "";
    snprintf(ssl_err_buf, sizeof(ssl_err_buf), "%s", ERR_reason_error_string(r) ?: "");
    if (*ssl_err_buf)
        return ssl_err_buf;
    return "";
}

/**
 * @description: 初始化相关SSL_CTX的结构指针
 * @param {int} method  0:server 1:client
 * @return {*}  返回SSL_CTX结构指针，使用 ssl_free_ctx() 释放内存
 */
SSL_CTX *ssl_socket_new_ctx(int method)
{
    SSL_CTX *ctx = NULL;
    /* SSL 库初始化 */
    SSL_library_init();
    /* 载入所有SSL算法 */
    OpenSSL_add_all_algorithms();
    /* 载入所有错误信息 */
    SSL_load_error_strings();
    /* 以SSL V2 和V3 标准兼容方式产生一个SSL_CTX ，即SSL Content Text */
    if (method == 0)
    {
        ctx = SSL_CTX_new(SSLv23_server_method());
        SSL_CTX_set_options(ctx, SSL_OP_SINGLE_DH_USE); // 使用临时 DH 参数时始终创建新密钥
    }
    else if (method == 1)
    {
        ctx = SSL_CTX_new(SSLv23_client_method());
    }

    if (ctx == NULL)
    {
        snprintf(socket_err, sizeof(socket_err), "SSL_CTX_new(SSLv23_client_method()) fail");
        return NULL;
    }

    // 设置可信CA证书的默认位置
    SSL_CTX_set_default_verify_paths(ctx);

    return ctx;
}
/**
 * @description: 设置可信CA的证书位置 （可选）
 * @param {SSL_CTX} *ctx
 * @param {char} *ca_file
 * @param {char} *ca_path
 * @return {*} 0:成功，1:失败
 * 如果不调用此函数，则会使用CA证书的默认位置和证书。
 * 默认的CA证书目录在默认的OpenSSL目录中称为certs，具体查看:
 *  https://www.openssl.org/docs/manmaster/man3/SSL_CTX_load_verify_locations.html
 */
int load_ca_cert(SSL_CTX *ctx, char *ca_file, char *ca_path)
{
    if (!SSL_CTX_load_verify_locations(ctx, ca_file, ca_path))
    {
        snprintf(socket_err, sizeof(socket_err), "SSL_CTX_load_verify_locations: %s", get_error_info());
        return 1;
    }
    return 0;
}
/**
 * @description:    设置我方的证书和私钥
 * @param {SSL_CTX} *ctx        指向由 ssl_socket_new_ctx 生成的对象
 * @param {char} *cert_file     证书文件，CA颁发给我们的证书
 * @param {char} *key_file      证书链文件和我们的私钥，该文件需要包含证书链和私钥内容，
 *                              可以使用以下命令生成:
 *                                  cat p1.crt p2.crt p3.crt serv.key > serv.pem
 * @param {char} *password      自己私钥的密码，如果没有写 NULL
 * @return {*}  -1:fail, 0:success
 */
int load_certificates(SSL_CTX *ctx, char *cert_file, char *key_file, char *password)
{
    if (load_ca_cert(ctx, cert_file, NULL))
    {
        snprintf(socket_err, sizeof(socket_err), "set we cert file fail");
        return -1;
    }

    /* 将证书链从文件加载到ctx中，证书必须采用PEM格式 */
    if (SSL_CTX_use_certificate_chain_file(ctx, key_file) != 1)
    // if (SSL_CTX_use_certificate_chain_file(ctx, cert_file) != 1)
    {
        snprintf(socket_err, sizeof(socket_err), "SSL_CTX_use_certificate_chain_file: %s", get_error_info());
        SSL_CTX_free(ctx);
        return -1;
    }

    if (password != NULL)
        SSL_CTX_set_default_passwd_cb_userdata(ctx, password);

    /* 载入私钥，将文件中找到的第一个私有RSA密钥添加到ctx */
    if (SSL_CTX_use_RSAPrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) != 1)
    {
        snprintf(socket_err, sizeof(socket_err), "SSL_CTX_use_RSAPrivateKey_file: %s", get_error_info());
        SSL_CTX_free(ctx);
        return -1;
    }
    /* 检查私钥与加载到ctx中的相应证书的一致性 */
    if (SSL_CTX_check_private_key(ctx) != 1)
    {
        snprintf(socket_err, sizeof(socket_err), "SSL_CTX_check_private_key: %s", get_error_info());
        SSL_CTX_free(ctx);
        return -1;
    }

    return 0;
}

/**
 * @description: 验证回调函数
 * @param {int} preverify_ok        是否通过了相关证书的验证: 1(通过), 0(未通过)
 * @param {X509_STORE_CTX} *ctx     指向用于证书链难的完整上下文的指针
 * @return {int}  返回值控制着进一步验证过程的策略。如果返回0，则验证过程立即以"验证失败"状态停止。
 * 如果设置了 SSL_VERIFY_PEER，则会向对方发送验证失败警报，并终止 TLS/SSL 握手。如果返回1，则继续验证。
 * 更多的错误类型查看: https://www.openssl.org/docs/man1.0.2/man3/X509_STORE_CTX_get_error.html
 */
static int verify_cb(int preverify_ok, X509_STORE_CTX *ctx)
{
    char buf[1024] = {0};
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
    X509_NAME_oneline(X509_get_issuer_name(cert), buf, sizeof(buf));
    printf("Verify Ceritficates:\n");
    printf("  issuer  = %s\n", buf);
    X509_NAME_oneline(X509_get_subject_name(cert), buf, sizeof(buf));
    printf("  subject = %s\n", buf);

    int cert_error = X509_STORE_CTX_get_error(ctx);
    printf("  SSL Verify Certificates res(%d) cert error(%d)\n", preverify_ok, cert_error);
    printf("------\n");
    if (!preverify_ok)
    {
        int depth = X509_STORE_CTX_get_error_depth(ctx);
        int err = X509_STORE_CTX_get_error(ctx);

        switch (cert_error)
        {
        case X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER:
        case X509_V_ERR_KEYUSAGE_NO_CRL_SIGN:           // 无法获取颁发者证书
        case X509_V_ERR_DIFFERENT_CRL_SCOPE:            // 唯一可以找到的 CRL 与证书的范围不匹配
        case X509_V_ERR_CRL_PATH_VALIDATION_ERROR:      // CRL 路径验证错误
        case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD: // CRL lastUpdate 字段包括无效时间
        case X509_V_ERR_CRL_NOT_YET_VALID:              // CRL 尚未生效
        case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD: // CRL nextUpdate 字段包含无效时间
        case X509_V_ERR_CRL_HAS_EXPIRED:                // CRL 已过期
        case X509_V_ERR_CRL_SIGNATURE_FAILURE:          // CRL 签名无效
        case X509_V_ERR_UNABLE_TO_GET_CRL:              // 找不到证书的 CRL
            preverify_ok = 1;
            /* 要求: 证书撤销列表本地的错误，不应该导致客户端连接不成功 */
            /* 检查证书的时候，若失败，线程投递错误，导致ssl_read失败 */

            break;
        default:
            printf("-Error with certificate at depth: %d\n", depth);
            printf("  err %i:%s\n", err, X509_verify_cert_error_string(err));
            snprintf(socket_err, sizeof(socket_err), "err %i:%s", err, X509_verify_cert_error_string(err));
            break;
        }
    }

    /* 在回调中修改了函数返回值，让crl的错误返回成功，但是错误还投递了，这里让他清空一下。*/
    ERR_clear_error();

    return preverify_ok;
}
/**
 * @description: 设置是否验证对方证书
 * @param {SSL_CTX} *ctx
 * @param {int} bit         SSL_SOCKET_VERIFY_NONE 或 SSL_SOCKET_VERIFY
 * @param {int} depth       证书链的深度
 * @return {*}
 */
void ssl_socket_set_verify_certificates(SSL_CTX *ctx, int bit, int depth)
{
    // 设置证书验证模式和回调
    /**
     * SSL_VERIFY_NONE:
     *  @server mode: 服务器不会向客户端发送客户端证书请求，因此客户端不会发送证书。
     *  @client mode: 如果不使用匿名密码（默认禁用），服务器将发送一个将被检查的证书。
     *              在 TLS/SSL 握手之后，可以使用SSL_get_verify_result(3)函数
     *              检查证书验证过程的结果。无论验证结果如何，都会继续握手。
     *
     * SSL_VERIFY_PEER:
     *  @Server mode: 服务器向客户端发送客户端证书请求。检查返回的证书（如果有）。
     *              如果验证过程失败，TLS/SSL 握手会立即终止，并发出一条包含验证失败
     *              原因的警报消息。该行为可以通过附加的 SSL_VERIFY_FAIL_IF_NO_PEER_CERT
     *              和 SSL_VERIFY_CLIENT_ONCE 标志来控制。
     *  @Client mode: 验证服务器证书。如果验证过程失败，TLS/SSL 握手会立即终止，并
     *              发出一条包含验证失败原因的警报消息。如果没有发送服务器证书，因为使用
     *              了匿名密码，SSL_VERIFY_PEER 将被忽略。
     *
     * SSL_VERIFY_FAIL_IF_NO_PEER_CERT:
     *  @Server mode: 如果客户端没有返回证书，TLS/SSL 握手会立即终止，并发出“握手失败”警报。
     *              此标志必须与 SSL_VERIFY_PEER 一起使用。
     *  @Client mode: 忽略
     *
     * SSL_VERIFY_CLIENT_ONCE:
     *  @Server mode: 仅在初始 TLS/SSL 握手时请求客户端证书。在重新协商的情况下，不要再次要求
     *              客户证书。此标志必须与 SSL_VERIFY_PEER 一起使用。
     *  @Client mode: 忽略
     *
     * @note: 必须随时设置模式标志 SSL_VERIFY_NONE 和 SSL_VERIFY_PEER之一。
     */
    if (bit == SSL_SOCKET_VERIFY_NONE)
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
    else
    {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, verify_cb);
        // 设置证书链的验证最大深度
        SSL_CTX_set_verify_depth(ctx, depth);
    }
}
/**
 * @description:    设置可用的算法列表
 * @param {SSL_CTX} *ctx
 * @param {char} *ciphers   可用的算法列表 (ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH)
 * @return {*}  成功返回1，失败返回0
 */
int ssl_socket_set_cipher_list_serv(SSL_CTX *ctx, const char *ciphers)
{
    if (!ciphers || !*ciphers)
        return SSL_CTX_set_cipher_list(ctx, CIPHER_LIST);
    else
        return SSL_CTX_set_cipher_list(ctx, ciphers);
}
/**
 * @description:    设置可用的算法列表
 * @param {SSL_CTX} *ctx        由 ssl_socket_new_ctx 生成的ctx
 * @param {char} *cipher_file   写有算法的文件名
 * @return {*}  成功返回0, 失败返回-1
 */
int ssl_socket_set_cipher_file_serv(SSL_CTX *ctx, char *cipher_file)
{
    char *cipher = NULL;
    struct stat sb;
    if (stat(cipher_file, &sb) == -1)
    {
        snprintf(socket_err, sizeof(socket_err), "stat file(%s) fail:%s", cipher_file, strerror(errno));
        return -1;
    }
    cipher = (char *)malloc(sb.st_size + 1);
    if (cipher == NULL)
    {
        snprintf(socket_err, sizeof(socket_err), "malloc %lld memory fail:%s", sb.st_size + 1, strerror(errno));
        return -1;
    }
    FILE *fp = fopen(cipher_file, "r");
    if (!fp)
    {
        free(cipher);
        snprintf(socket_err, sizeof(socket_err), "open file(%s) fail:%s", cipher_file, strerror(errno));
        return -1;
    }
    size_t n = fread(cipher, sb.st_size, 1, fp);
    if (n != 1)
    {
        free(cipher);
        fclose(fp);
        snprintf(socket_err, sizeof(socket_err), "read file(%s) fail:%s", cipher_file, strerror(errno));
        return -1;
    }
    fclose(fp);

    // convert all '\0's except the last one to ':'
    int i = 0;
    for (i = 0; i < sb.st_size - 1; i++)
    {
        if (!cipher[i])
            cipher[i] = ':';
    }

    n = ssl_socket_set_cipher_list_serv(ctx, cipher);

    free(cipher);

    return (n == 0) ? -1 : 0;
}
static char dhdir[1024] = {0};
DH *tmp_dh_cb(SSL *ssl, int is_export, int keylen)
{
    char dh_file[2048] = {0};

    if (keylen == 512)
    {
        snprintf(dh_file, sizeof(dh_file), "%s/dh512.pem", dhdir);
        FILE *fp = fopen(dh_file, "r");
        if (fp)
        {
            DH *dh = PEM_read_DHparams(fp, NULL, NULL, NULL);
            fclose(fp);
            if (dh)
                return dh;
        }
    }
    if (keylen == 1024)
    {
        snprintf(dh_file, sizeof(dh_file), "%s/dh1024.pem", dhdir);
        FILE *fp = fopen(dh_file, "r");
        if (fp)
        {
            DH *dh = PEM_read_DHparams(fp, NULL, NULL, NULL);
            fclose(fp);
            if (dh)
                return dh;
        }
    }
    return DH_generate_parameters(keylen, DH_GENERATOR_2, NULL, NULL);
}
/**
 * @description:    设置为临时密钥交换处理 DH 密钥
 * @param {SSL_CTX} *ctx
 * @param {char} *dh_path   保存dh密钥的目录，目录下应该有: dh512.pem dh1024.pem 两个文件
 * @return {*}
 */
void ssl_socket_set_tmp_dh_path(SSL_CTX *ctx, char *dh_path)
{
    snprintf(dhdir, sizeof(dhdir), "%s", dh_path);
    SSL_CTX_set_tmp_dh_callback(ctx, tmp_dh_cb);
}
/**
 * @description:    释放由ssl_new_ctx生成的指针
 * @param {SSL_CTX} *ctx
 * @return {*}
 */
void ssl_socket_free_ctx(SSL_CTX *ctx)
{
    if (ctx != NULL)
    {
        SSL_CTX_free(ctx);
        ctx = NULL;
    }
}
/**
 * @description:    释放由ssl_socket_connect生成的ssl *
 * @param {SSL} *ssl    指定被释放的ssl *
 * @return {*}
 */
void ssl_socket_cleanup(SSL *ssl)
{
    // 线程局部清理
    ERR_remove_state(0);

    // sk_SSL_COMP_free(SSL_COMP_get_compression_methods());

    if (ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = NULL;
    }

    // 全局应用程序退出清理
    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    // SSL_free_comp_methods();
}

static datetime_sec now()
{
    return time((long *)0);
}
// return: -1(fail), >0:成功
static int ssl_timeoutio(int (*fun)(), SSL *ssl, int rfd, int wfd, int timeout, char *buf, size_t buf_len)
{
    int n;
    const datetime_sec end = (datetime_sec)timeout + now();

    ERR_clear_error();

    do
    {
        fd_set fds;
        struct timeval tv;

        const int r = buf ? fun(ssl, buf, buf_len) : fun(ssl);
        if (r > 0)
            return r;

        timeout = end - now();
        if (timeout < 0)
            break;
        tv.tv_sec = (time_t)timeout;
        tv.tv_usec = 0;

        FD_ZERO(&fds);
        switch (SSL_get_error(ssl, r))
        {
        default:
            return r; // some other error
        case SSL_ERROR_WANT_READ:
            FD_SET(rfd, &fds);
            n = select(rfd + 1, &fds, NULL, NULL, &tv);
            break;
        case SSL_ERROR_WANT_WRITE:
            FD_SET(wfd, &fds);
            n = select(wfd + 1, NULL, &fds, NULL, &tv);
            break;
        }

        // n is the number of descriptors that changed status
    } while (n > 0);

    if (n != -1)
        errno = ETIMEDOUT;

    snprintf(socket_err, sizeof(socket_err), "%s", get_error_info());
    return -1;
}

/**
 * @description:    对指定的fd生成相应的ssl并connect(client端)
 * @param {SSL_CTX} *ctx    使用ssl_new_ctx生成的相关结构
 * @param {int} fd          指定的socket fd
 * @param {int} timeout     设置连接超时时间
 * @return {*}  返回生成的ssl, 使用ssl_socket_cleanup()回收内存,失败返回NULL
 */
SSL *ssl_socket_connect(SSL_CTX *ctx, int fd, int timeout)
{
    int r = -1;
    SSL *ssl = NULL;

    // 基于ctx 产生一个新的SSL
    ssl = SSL_new(ctx);
    if (ssl == NULL)
    {
        snprintf(socket_err, sizeof(socket_err), "unable to create SSL instance %s", get_error_info());
        // ssl_socket_free_ctx(ctx);
        return NULL;
    }

    // if connection is established, keep NDELAY
    if (ndelay_on(fd) == -1)
    {
        snprintf(socket_err, sizeof(socket_err), "set fd to nonblock fail: %s", strerror(errno));
        // ssl_socket_free_ctx(ctx);
        ssl_socket_cleanup(ssl);
        return NULL;
    }

    // 将连接的socket加入到SSL
    SSL_set_fd(ssl, fd);

    // 建立SSL连接
    r = ssl_timeoutio(SSL_connect, ssl, fd, fd, timeout, NULL, 0);
    if (r <= 0)
    {
        ndelay_off(fd);

        if (errno == ETIMEDOUT)
            snprintf(socket_err, sizeof(socket_err), "ssl connect timeout");

        // ssl_socket_free_ctx(ctx);
        ssl_socket_cleanup(ssl);
        return NULL;
    }
    SSL_set_mode(ssl, SSL_MODE_ENABLE_PARTIAL_WRITE);

    return ssl;
}

/**
 * @description:    对指定的fd生成相应的ssl并accept(server端)
 * @param {SSL_CTX} *ctx
 * @param {int} fd
 * @param {int} timeout
 * @return {*}
 */
SSL *ssl_socket_accept(SSL_CTX *ctx, int fd, int timeout)
{
    int r = -1;
    SSL *ssl = NULL;
    BIO *sbio = NULL;
    int ssl_rfd, ssl_wfd;

    // 基于ctx产生一个新的SSL
    ssl = SSL_new(ctx);
    if (ssl == NULL)
    {
        snprintf(socket_err, sizeof(socket_err), "unable to create SSL instance %s", get_error_info());
        return NULL;
    }

    sbio = BIO_new_socket(fd, BIO_NOCLOSE);
    if (!sbio)
    {
        snprintf(socket_err, sizeof(socket_err), "BIO_new_socket %s", get_error_info());
        ssl_socket_cleanup(ssl);
        return NULL;
    }
    SSL_set_bio(ssl, sbio, sbio); // 将SSL对象与BIO连接

    SSL_set_rfd(ssl, ssl_rfd = fd);
    SSL_set_wfd(ssl, ssl_wfd = fd);

    // if connection is established, keep NDELAY
    if (ndelay_on(fd) == -1)
    {
        snprintf(socket_err, sizeof(socket_err), "set fd to nonblock fail: %s", strerror(errno));
        // ssl_socket_free_ctx(ctx);
        ssl_socket_cleanup(ssl);
        return NULL;
    }

    // 将连接的socket加入到SSL
    // SSL_set_fd(ssl, fd);

    // 建立SSL连接
    r = ssl_timeoutio(SSL_accept, ssl, fd, fd, timeout, NULL, 0);
    if (r <= 0)
    {
        ndelay_off(fd);
        if (r == -1)
        {
            snprintf(socket_err, sizeof(socket_err), "unable to SSL accept %s", get_error_info());
            // ssl_socket_free_ctx(ctx);
            ssl_socket_cleanup(ssl);

            return NULL;
        }
    }
    else
        SSL_set_mode(ssl, SSL_MODE_ENABLE_PARTIAL_WRITE);

    return ssl;
}

/**
 * @description:    使用ssl读取数据
 * @param {SSL} *ssl        加密ssl
 * @param {int} fd          非加密的socket fd
 * @param {char} *buf       用于保存数据的buffer
 * @param {size_t} buf_size buffer的空间大小
 * @param {int} timeout     读取的超时时间
 * @return {*}  返回读取的数据的长度, 0:对方断开连接, -1:失败, 如 errno==ETIMEDOUT 则为超时
 */
int ssl_socket_read(SSL *ssl, int fd, char *buf, size_t buf_size, int timeout)
{
    if (!buf)
    {
        snprintf(socket_err, sizeof(socket_err), "not enought memory space of buf for store data");
        return -1;
    }

    if (SSL_pending(ssl))
        return SSL_read(ssl, buf, buf_size);
    return ssl_timeoutio(SSL_read, ssl, fd, fd, timeout, buf, buf_size);
}
/**
 * @description:    使用ssl写入数据
 * @param {SSL} *ssl        加密ssl
 * @param {int} fd          非加密的socket fd
 * @param {char} *buf       用于发送数据的buffer
 * @param {size_t} buf_len  buffer的长度
 * @param {int} timeout     写入的超时时间
 * @return {*}
 */
int ssl_socket_write(SSL *ssl, int fd, char *buf, size_t buf_len, int timeout)
{
    if (!buf)
    {
        return 0;
    }
    return ssl_timeoutio(SSL_write, ssl, fd, fd, timeout, buf, buf_len);
}

/**
 * @description: 显示证书信息
 * @param {SSL} *ssl
 * @return {*}
 */
void show_certs(SSL *ssl)
{
    X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl); // Get certificates (if available)
    if (cert != NULL)
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line); // 证书颁发者
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificates.\n");
}

#endif
