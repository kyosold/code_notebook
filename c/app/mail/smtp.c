#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <getopt.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "base64.h"
#include "sio.h"
#include "sio_ssl.h"

#define MAX_LINE 1024

typedef struct sparam
{
    char *host;
    char *sasl;
    char *bindip;
    char *from;
    char *to;
    char *eml;
    unsigned int istls;
} sparam;

enum STAGE
{
    GREET,
    HELO,
    EHLO,
    AUTH,
    USER,
    PASSWD,
    MAIL,
    RCPT,
    DATA,
    EOM,
    QUIT,
    CLOSE
};

static char stage_str[15][15] = {"GREET",
                                 "HELO",
                                 "EHLO",
                                 "AUTH",
                                 "USER",
                                 "PASS",
                                 "MAIL",
                                 "RCPT",
                                 "DATA",
                                 "EOM",
                                 "QUIT",
                                 "CLOSE"};

struct sparam sp;
int sockfd = -1;
SSL_CTX *ctx = NULL;
SSL *ssl = NULL;
char err[1024] = {0};
size_t err_size = sizeof(err);

void usage(char *prog)
{
    printf("Usage:\n");
    printf("  %s -h[host:port] -s -a[user:password] -f[from] -t[to] -e[eml] -b[bind_ip:helo_name]\n", prog);
    printf("参数说明:\n");
    printf("  -h 连接的远程服务器地址和端口,host:port\n");
    printf("  -s 使用TLS加密传输\n");
    printf("  -a 认证用户名和密码,user:password, 如果不需要认证，不写此参数\n");
    printf("  -f 信封发件人地址，必须为邮件地址\n");
    printf("  -t 信封收件人地址，必须为邮件地址，多个收件人中间用逗号(,)分割\n");
    printf("  -e eml邮件源码文件，可以使用 s-generate_eml 生成\n");
    printf("  -b 绑定指定的本地ip和发送helo name，一般helo name和本地对应的IP解板一致，如果不需要，可以不写此参数\n");
    printf("\n---------------------------\n例子:\n");
    printf("[1] %s -h'163mx02.mxmail.netease.com:25' -f'kyosold@qq.com' -t'kyosold@163.com' -e'./test.eml'\n", prog);
}

void sparam_init(struct sparam *sp)
{
    sp->host = NULL;
    sp->sasl = NULL;
    sp->bindip = NULL;
    sp->from = NULL;
    sp->to = NULL;
    sp->eml = NULL;
    sp->istls = 0;
}

void sparam_clean(struct sparam *sp)
{
    if (sp->host)
        free(sp->host);
    if (sp->sasl)
        free(sp->sasl);
    if (sp->bindip)
        free(sp->bindip);
    if (sp->from)
        free(sp->from);
    if (sp->to)
        free(sp->to);
    if (sp->eml)
        free(sp->eml);

    sparam_init(sp);
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

ssize_t sread(char *buf, size_t len)
{
    char err[1024] = {0};
    if (sp.istls)
        return sock_ssl_read(ssl, sockfd, buf, len, 3, err, sizeof(err));
    else
        return sock_read(sockfd, buf, len, 3);
}

ssize_t swrite(char *buf, size_t len)
{
    if (sp.istls)
        return sock_ssl_write(ssl, sockfd, buf, len, 3, err, err_size);
    else
        return sock_write(sockfd, buf, len, 3);
}

/**
 * @brief
 * @return 0:succ, other is error.
 */
int blast(char *eml_fs, int sockfd, size_t *slen)
{
    size_t s_len = 0;
    int is_endline = 1;
    char sbuf[4096] = {0};
    char *p_sbuf = NULL;
    char body_buf[8192] = {0};
    char *p_body_buf = NULL;
    unsigned int body_buf_i = 0;
    int rn = 0, m = 0;

    int fd_eml = open(eml_fs, O_RDONLY);
    if (fd_eml == -1)
    {
        printf("open eml file %s fail: %s\n", eml_fs, strerror(errno));
        return 1;
    }

    while ((rn = read(fd_eml, sbuf, sizeof(sbuf) - 1)) > 0)
    {
        sbuf[rn] = '\0';
        p_sbuf = sbuf;

        memset(body_buf, 0, sizeof(body_buf));
        p_body_buf = body_buf;
        body_buf_i = 0;

        m = 0;
        if (rn > 0)
        {
            // 处理.开头的, 并且把 \n 换成 \r\n
            for (;;)
            {
                if (m >= rn || sbuf[m] == '\0')
                    break;

                // 以.开头的要添加一个.
                if (is_endline == 1 && sbuf[m] == '.')
                    body_buf[body_buf_i++] = sbuf[m];

                // 处理一行，结尾使用 \r\n
                while (m < rn && sbuf[m] != '\0')
                {
                    if (sbuf[m] != '\n')
                    {
                        if (sbuf[m] != '\r')
                            body_buf[body_buf_i++] = sbuf[m];
                        m++;
                    }
                    else
                    {
                        body_buf[body_buf_i++] = '\r';
                        body_buf[body_buf_i++] = '\n';
                        m++;
                        break;
                    }
                }

                if (body_buf[body_buf_i - 1] == '\n')
                    is_endline = 1;
                else
                    is_endline = 0;
            }
        }

        while (body_buf_i > 0)
        {
            int wn = swrite(p_body_buf, body_buf_i);
            if (wn <= 0)
            {
                if (errno == EINTR)
                    wn = 0;
                else
                {
                    printf("> Write body to remote fail: %s\n", strerror(errno));
                    goto SFAIL;
                }
            }
            body_buf_i -= wn;
            p_body_buf += wn;

            s_len += wn;
        }

        memset(sbuf, 0, sizeof(sbuf));
    }

    close(fd_eml);

    *slen = s_len;

    return 0;

SFAIL:
    if (fd_eml)
        close(fd_eml);

    return 1;
}

/**
 * @brief 获取rcode, 并判断
 * @return 0:succ, 1:error
 * 
 * 因为有多行的，只取最后一行的
 */
unsigned int smtp_code(enum STAGE stage, char *buf, size_t buf_len)
{
    char rcode[4] = {0};
    unsigned int code = 0;
    int i = buf_len - 2;
    while ((buf[i] != '\n') && (i > 0))
        i--;

    if (i == 0)
        strncpy(rcode, buf, 3);
    else
        strncpy(rcode, &buf[i + 1], 3);

    rcode[3] = '\0';

    code = atoi(rcode);

    // 判断
    switch (stage)
    {
    case GREET:
        if (code != 220)
        {
            printf("! [%s] code:%d want(220) error.\n", stage_str[stage], code);
            goto SFAIL;
        }
        break;
    case HELO:
    case EHLO:
    case MAIL:
    case RCPT:
    case EOM:
        if (code != 250)
        {
            printf("! [%s] code:%d want(250) error.\n", stage_str[stage], code);
            goto SFAIL;
        }
        break;
    case AUTH:
    case USER:
        if (code != 334)
        {
            printf("! [%s] code:%d want(334) error.\n", stage_str[stage], code);
            goto SFAIL;
        }
        break;
    case PASSWD:
        if (code != 235)
        {
            printf("! [%s] code:%d want(235) error.\n", stage_str[stage], code);
            goto SFAIL;
        }
        break;
    case DATA:
        if (code != 354)
        {
            printf("! [%s] code:%d want(354) error.\n", stage_str[stage], code);
            goto SFAIL;
        }
        break;
    case QUIT:
        if (code != 221)
        {
            printf("! [%s] code:%d want(221) error.\n", stage_str[stage], code);
            goto SFAIL;
        }
        break;
    }

    return 0;

SFAIL:
    return 1;
}

int main(int argc, char **argv)
{
    int i = 0;

    sparam_init(&sp);

    int ch;
    const char *args = "h:a:f:t:e:b:s";
    while ((ch = getopt(argc, argv, args)) != -1)
    {
        switch (ch)
        {
        case 'h':
            sp.host = strdup(optarg);
            i++;
            break;
        case 'a':
            sp.sasl = strdup(optarg);
            break;
        case 'f':
            sp.from = strdup(optarg);
            break;
        case 't':
            i++;
            sp.to = strdup(optarg);
            break;
        case 'e':
            i++;
            sp.eml = strdup(optarg);
            break;
        case 'b':
            sp.bindip = strdup(optarg);
            break;
        case 's':
            sp.istls = 1;
            break;
        }
    }

    if (argc < 3)
    {
        usage(argv[0]);
        return 1;
    }

    char host[MAX_LINE] = {0};
    char port[MAX_LINE] = {0};
    char suser[MAX_LINE] = {0};
    char *suser_b64 = NULL;
    char spwd[MAX_LINE] = {0};
    char *spwd_b64 = NULL;
    char bip[16] = {0};
    char bhelo[MAX_LINE] = {0};
    char *to_tok = sp.to;
    char *tok = NULL;

    if (sp.host)
        sscanf(sp.host, "%[^:]:%s", host, port);
    if (sp.sasl)
    {
        sscanf(sp.sasl, "%[^:]:%s", suser, spwd);
        int len = 0;
        suser_b64 = sbase64_encode_alloc(suser, strlen(suser), &len);
        spwd_b64 = sbase64_encode_alloc(spwd, strlen(spwd), &len);
    }
    if (sp.bindip)
        sscanf(sp.bindip, "%[^:]:%s", bip, bhelo);

    if (*(sp.to + strlen(sp.to)) == ',')
        *(sp.to + strlen(sp.to)) = '\0';

    printf("==================================\n");
    printf("  host: %s\n", host);
    printf("  port: %s\n", port);
    printf("  user: %s\n", suser);
    printf("  pass: %s\n", spwd);
    printf("  bind ip: %s\n", bip);
    printf("  bind name: %s\n", bhelo);
    printf("==================================\n");

    // int sockfd = -1;
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
        goto SFAIL;
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
        goto SFAIL;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
    printf("> Connect to %s:%s succ.\n", s, port);

    freeaddrinfo(servinfo);

    char hostname[HOST_NAME_MAX + 1] = {0};
    gethostname(hostname, sizeof(hostname));

    if (sp.istls)
    {
        ctx = sock_ssl_ctx_new(1, NULL, NULL, err, err_size);
        if (!ctx)
        {
            printf("SSL Fail: %s\n", err);
            goto SFAIL;
        }

        ssl = sock_ssl_connect(ctx, sockfd, err, err_size);
        if (!ssl)
        {
            printf("SSL Fail: %s\n", err);
            goto SFAIL;
        }
    }

    // Read/Write ---------------------------
    char buf[4096] = {0};  // 4k
    char sbuf[4096] = {0}; // 4k
    char *p_sbuf = NULL;
    char rcode[5] = {0};

    size_t slen = 0;
    int ri = 0;
    int rn = 0;
    enum STAGE stage = GREET;

    while ((rn = sread(buf, sizeof(buf))) > 0)
    {
        buf[rn] = 0;
        printf("< %s\n", buf);

        if (smtp_code(stage, buf, rn) != 0)
            goto SFAIL;

        switch (stage)
        {
        case GREET:
            if (sp.sasl)
            {
                snprintf(sbuf, sizeof(sbuf), "EHLO %s\r\n", hostname);
                stage = EHLO;
            }
            else
            {
                snprintf(sbuf, sizeof(sbuf), "HELO %s\r\n", hostname);
                stage = HELO;
            }
            break;
        case PASSWD:
        case HELO:
            snprintf(sbuf, sizeof(sbuf), "MAIL FROM:<%s>\r\n", sp.from);
            stage = MAIL;
            break;
        case EHLO:
            snprintf(sbuf, sizeof(sbuf), "AUTH LOGIN\r\n");
            stage = AUTH;
            break;
        case AUTH:
            snprintf(sbuf, sizeof(sbuf), "%s\r\n", suser_b64);
            stage = USER;
            break;
        case USER:
            snprintf(sbuf, sizeof(sbuf), "%s\r\n", spwd_b64);
            stage = PASSWD;
            break;
        case MAIL:
            tok = (char *)memchr(to_tok, ',', strlen(to_tok));
            if (tok)
            {
                *tok = 0;
                snprintf(sbuf, sizeof(sbuf), "RCPT TO:<%s>\r\n", to_tok);
                *tok = ',';
                to_tok = tok + 1;
            }
            else
            {
                snprintf(sbuf, sizeof(sbuf), "RCPT TO:<%s>\r\n", to_tok);
                stage = RCPT;
            }
            break;
        case RCPT:
            snprintf(sbuf, sizeof(sbuf), "DATA\r\n");
            stage = DATA;
            break;
        case DATA:
            if (blast(sp.eml, sockfd, &slen) != 0)
                goto SFAIL;
            printf("> [Send %lld bytes]\n", slen);
            snprintf(sbuf, sizeof(sbuf), "\r\n.\r\n", 5);
            stage = EOM;
            break;
        case EOM:
            snprintf(sbuf, sizeof(sbuf), "QUIT\r\n");
            stage = QUIT;
            break;
        case QUIT:
            goto SOK;
            break;
        }

        printf("> %s", sbuf);
        swrite(sbuf, strlen(sbuf));
    }

    if (rn == -1)
    {
        printf("> read %s:%s fail: %s\n", host, port, strerror(errno));
        goto SFAIL;
    }
    else if (rn == 0)
    {
        printf("> remote %s:%s closed.\n", host, port);
        goto SFAIL;
    }

SOK:
    close(sockfd);
    sparam_clean(&sp);

    if (suser_b64)
        free(suser_b64);
    if (spwd_b64)
        free(spwd_b64);

    if (ssl)
        sock_ssl_cleanup(ssl);
    if (ctx)
        sock_ssl_ctx_free(ctx);

    printf("> Close remote. bye~ \n");

    return 0;

SFAIL:
    printf("! Close remote.\n");
    if (sockfd != -1)
        close(sockfd);

    sparam_clean(&sp);

    if (suser_b64)
        free(suser_b64);
    if (spwd_b64)
        free(spwd_b64);

    if (ssl)
        sock_ssl_cleanup(ssl);
    if (ctx)
        sock_ssl_ctx_free(ctx);

    return 1;
}
