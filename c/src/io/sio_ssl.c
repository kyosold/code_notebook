#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include "sio_ssl.h"

#ifdef _DEBUG
int _debug = 1;
#else
int _debug = 0;
#endif

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

int ndelay_on(int fd)
{
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

int ndelay_off(int fd)
{
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
}

char ssl_err_buf[1024] = {0};
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

void sock_ssl_cleanup(SSL *ssl)
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

/**
 * @brief Check certificate
 * @param ssl
 * @return -2:无证书, -1:验证失败, 0:验证成功
 */
int check_certs(SSL *ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl);
    // SSL_get_verify_result()是重点，SSL_CTX_set_verify()只是配置启不启用并没有执行认证，调用该函数才会真证进行证书认证
    // 如果验证不通过，那么程序抛出异常中止连接
    if (SSL_get_verify_result(ssl) == X509_V_OK)
    {
        // 证书验证通过
        return 0;
    }

    if (cert != NULL)
    {
        if (_debug)
        {
            printf("数字证书信息:\n");
            line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
            printf("证书: %s\n", line);
            line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
            printf("颁发者: %s\n", line);
            free(line);
            X509_free(cert);
        }
        return -1;
    }
    else
        return -2;
}

/**
 * @brief 设置使用双向验证，即：验证客户端的证书
 * @param ctx       Pointer to SSL_CTX object
 * @param cafile    The CA certificate(*.crt)
 * @param err       The error string
 * @param err_size  The memory size of err
 * @return 0:succ, -1:fail and check err
 */
int sock_ssl_use_2way_auth(SSL_CTX *ctx, char *cafile, char *err, size_t err_size)
{
    // 双向验证
    // SSL_VERIFY_PEER---要求对证书进行认证，没有证书也会放行
    // SSL_VERIFY_FAIL_IF_NO_PEER_CERT---要求客户端需要提供证书，但验证发现单独使用没有证书也会放行
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    // 设置信任根证书
    if (SSL_CTX_load_verify_locations(ctx, cafile, NULL) <= 0)
    {
        snprintf(err, err_size,
                 "SSL_CTX_load_verify_locations: %s",
                 get_error_info());
        return -1;
    }
    return 0;
}

/**
 * @brief Creates a new SSL_CTX object
 * @param type      The special is server or client: 0(server), 1(client)
 * @param crtfile   The certificate file
 * @param keyfile   The Private Key file
 * @param err       The error string
 * @param err_size  The memory size of err 
 * @return Pointer to an SSL_CTX object, it return NULL when fail and check err.
 * 
 * to free ctx use sock_ssl_ctx_free()
 */
SSL_CTX *sock_ssl_ctx_new(int type, char *crtfile, char *keyfile, char *err, size_t err_size)
{
    SSL_CTX *ctx = NULL;

    // SSL library initialisation -----
    SSL_library_init();
    // load & register all cryptos, etc
    OpenSSL_add_all_algorithms();
    // load all error messages
    SSL_load_error_strings();

    // ERR_load_BIO_strings();
    // ERR_load_crypto_strings();

    // 以SSL V2 和V3 标准兼容方式产生一个SSL_CTX ，即SSL Content Text
    if (type == 0)
        ctx = SSL_CTX_new(SSLv23_server_method());
    else
        // ctx = SSL_CTX_new(TLSv1_2_client_method());
        ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ctx)
    {
        snprintf(err, err_size, "SSL_CTX_new: %s", get_error_info());
        return NULL;
    }

    if (crtfile != NULL && keyfile != NULL)
    {
        // Load certificate and private key files, and check consistency
        // 载入服务器的数字证书， 此证书用来发送给客户端。 证书里包含有公钥
        if (!SSL_CTX_use_certificate_file(ctx, crtfile, SSL_FILETYPE_PEM))
        {
            snprintf(err, err_size,
                     "SSL_CTX_use_certificate_file: %s",
                     get_error_info());
            goto SFAIL;
        }
        if (_debug)
            printf("certificate file loaded ok\n");

        // Indicate the key file to be used
        // 载入服务器的钥
        if (!SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM))
        {
            snprintf(err, err_size,
                     "SSL_CTX_use_PrivateKey_file: %s",
                     get_error_info());
            goto SFAIL;
        }

        // Make sure the key and certificate file match.
        // 检查服务器私钥是否正确
        if (SSL_CTX_check_private_key(ctx) != 1)
        {
            snprintf(err, err_size,
                     "SSL_CTX_check_private_key: %s",
                     get_error_info());
            goto SFAIL;
        }
        if (_debug)
            printf("private key verified ok\n");
    }

    // Recommended to avoid SSLv2 & SSLv3
    // SSL_CTX_set_options(ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
    // SSL_CTX_set_options(ctx, SSL_OP_SINGLE_DH_USE);

    return ctx;

    // ssl = SSL_new(ctx);
    // if (!ssl)
    // {
    //     snprintf(err, err_size,
    //              "SSL_new: %s",
    //              get_error_info());
    //     goto SFAIL;
    // }

    // SSL_CTX_free(ctx);
    // ctx = NULL;

    // return ssl;

SFAIL:
    if (ctx)
    {
        SSL_CTX_free(ctx);
        ctx = NULL;
    }
    return NULL;
}

/**
 * @brief free an allocated SSL_CTX object
 * @param ctx   Pointer to SSL_CTX object
 * @return none
 */
void sock_ssl_ctx_free(SSL_CTX *ctx)
{
    if (ctx)
        SSL_CTX_free(ctx);
}

/**
 * @brief Create SSL object for the client fd
 * @param ctx       Pointer to SSL_CTX object
 * @param fd        The client fd
 * @param err       The error string
 * @param err_size  The memory size of err
 * @return Pointer to an SSL object, it return NULL when fail and check err
 * 
 * to free ssl use sock_ssl_cleanup()
 */
SSL *sock_ssl_new_client(SSL_CTX *ctx, int fd, char *err, size_t err_size)
{
    SSL *ssl = NULL;

    // 于 ctx 产生一个新的 SSL
    ssl = SSL_new(ctx);
    if (!ssl)
    {
        snprintf(err, err_size,
                 "SSL_new: %s",
                 get_error_info());
        goto SFAIL;
    }
    if (_debug)
        printf("create ssl succ\n");

    // 将连接用户的 socket 加入到 SSL
    SSL_set_fd(ssl, fd);
    if (_debug)
        printf("SSL_setfd succ\n");

    if (SSL_accept(ssl) == -1)
    {
        snprintf(err, err_size,
                 "SSL_accept");
        goto SFAIL;
    }
    if (_debug)
        printf("SSL_accept succ\n");

    return ssl;

SFAIL:
    if (ssl)
        sock_ssl_cleanup(ssl);

    return NULL;
}

/**
 * @brief free an allocated SSL structure
 * @param ssl   Pointer to SSL object
 * @return none
 */
void sock_ssl_free_client(SSL *ssl)
{
    sock_ssl_cleanup(ssl);
}

/**
 * @brief   Initiate the TLS/SSL handshake with an TLS/SSL server
 * @param ctx       Pointer to SSL_CTX object
 * @param fd        The server socket fd
 * @param err       The error string
 * @param err_size  The memory size of err
 * @return Pointer to an SSL object, it return NULL when fail and check err
 * 
 * to free ssl use sock_ssl_cleanup()
 */
SSL *sock_ssl_connect(SSL_CTX *ctx, int fd, char *err, size_t err_size)
{
    SSL *ssl = NULL;
    ssl = SSL_new(ctx);
    if (!ssl)
    {
        snprintf(err, err_size,
                 "SSL_new: %s",
                 get_error_info());
        goto SFAIL;
    }

    // 将连接用户的 socket 加入到 SSL
    SSL_set_fd(ssl, fd);

    if (SSL_connect(ssl) == -1)
    {
        snprintf(err, err_size,
                 "SSL_connect: %s",
                 get_error_info());
        goto SFAIL;
    }

    return ssl;

SFAIL:
    if (ssl)
        sock_ssl_cleanup(ssl);

    return NULL;
}

/**
 * @brief read bytes from a TLS/SSL connection.
 * @param ssl       Point to SSL struct object
 * @param fd        Read from client fd
 * @param buf       Store to buffer
 * @param buf_size  The memory size of buf
 * @param timeout   Read expire seconds
 * @param err       The error string
 * @param err_size  The memory size of err
 * @return -2:timeout, -1:error and check err, other is read operation was successful.
 * 
 * sock_ssl_read() tries to read num bytes from the specified ssl into the buffer buf.
 */
int sock_ssl_read(SSL *ssl, int fd, char *buf, size_t buf_size, int timeout, char *err, size_t err_size)
{
    if (!ssl)
    {
        snprintf(err, err_size, "ssl is null, call sock_ssl_new first");
        return -1;
    }

    if (!buf)
        return 0;

    // 获取缓冲在 SSL 对象中的可读字节数
    if (SSL_pending(ssl))
        return SSL_read(ssl, buf, buf_size);

    int n = 0;
    const time_t end = time(NULL) + timeout;

    ERR_clear_error();

    do
    {
        fd_set fds;
        struct timeval tv;

        const int r = SSL_read(ssl, buf, buf_size);
        if (r > 0)
            return r;

        time_t t = end - time(NULL);
        if (t < 0)
            break;

        tv.tv_sec = t;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        switch (SSL_get_error(ssl, r))
        {
        case SSL_ERROR_WANT_READ:
            FD_SET(fd, &fds);
            n = select(fd + 1, &fds, NULL, NULL, &tv);
            break;
        default:
            return r;
        }

    } while (n > 0);

    if (n != -1) // timeout
        return -2;
    return -1;
}

/**
 * @brief write bytes to a TLS/SSL connection.
 * @param ssl       Point to SSL struct object
 * @param fd        Write to client fd
 * @param buf       Pointer to bytes to be written
 * @param buf_size  The number of the buf to be written
 * @param timeout   Read expire seconds
 * @param err       The error string
 * @param err_size  The memory size of err
 * @return -2:timeout, -1:error and check err, other is write operation was successful.
 * 
 * sock_ssl_wirte() writes num bytes from the buffer buf into the specified ssl connection.
 */
int sock_ssl_write(SSL *ssl, int fd, char *buf, size_t buf_size, int timeout, char *err, size_t err_size)
{
    if (!ssl)
    {
        snprintf(err, err_size, "ssl is null, call sock_ssl_new first");
        return -1;
    }

    if (!buf)
        return 0;

    int n = 0;
    const time_t end = timeout + time(NULL);

    ERR_clear_error();

    do
    {
        fd_set fds;
        struct timeval tv;

        const int r = SSL_write(ssl, buf, buf_size);
        if (r > 0)
            return r;

        time_t t = end - time(NULL);
        if (t < 0)
            break;

        tv.tv_sec = t;
        tv.tv_usec = 0;
        FD_ZERO(&fds);

        switch (SSL_get_error(ssl, r))
        {
        case SSL_ERROR_WANT_WRITE:
            FD_SET(fd, &fds);
            n = select(fd + 1, NULL, &fds, NULL, &tv);
            break;
        default:
            return r;
        }
    } while (n > 0);

    if (n != -1)
        return -2;
    return -1;
}

// **********************************************************
#ifdef _SER_TEST
// ----------------------------------------------------
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
    // sock_ssl_cleanup(ssl);
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
#endif

#ifdef _CLI_TEST
// ----------------------------------------------------
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

#endif
