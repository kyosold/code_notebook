#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include "sio.h"

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

/**
 * @brief initiate a connection on a socket with timeout
 * @param fd        socket fd
 * @param addr      The address specified by addr
 * @param addrlen   The addrlen argument specifies the size of addr
 * @param timeout   Set connect timeout seconds
 * @return 0:succ, -1:fail, -2:timeout
 * 
 * 此函数是系统函数'connect'的封装，在用connect时，用此函数时即可.
 */
int sock_connect(int fd, const struct sockaddr *addr, socklen_t addrlen, unsigned int timeout)
{
    char ch;
    fd_set wfds;
    struct timeval tv;

    if (ndelay_on(fd) == -1)
        return -1;

    if (connect(fd, addr, addrlen) == 0)
    {
        ndelay_off(fd);
        return 0;
    }

    if ((errno != EINPROGRESS) && (errno != EWOULDBLOCK))
        return -1;

    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    if (select(fd + 1, (fd_set *)0, &wfds, (fd_set *)0, &tv) == -1)
        return -1;

    if (FD_ISSET(fd, &wfds))
    {
        ndelay_off(fd);
        return 0;
    }

    return -2;
}

/**
 * @brief attempts to read count bytes from socket fd into the buf
 * @param timeout_s    The timeout seconds 
 * @return -2:timeout, -1:error, other is succ
 */
ssize_t sock_read(int sockfd, char *buf, unsigned int count, unsigned int timeout)
{
    int r = 0;
    for (;;)
    {
        r = safe_read(sockfd, buf, count, timeout);
        if (r == -1)
            if (errno == EAGAIN || errno == EINTR)
                continue;
        break;
    }
    return r;
}

/**
 * @brief   writes up to count bytes from buf to socket fd
 * @return -2:timeout, -1:error, other is succ
 */
ssize_t sock_write(int sockfd, char *buf, unsigned int count, unsigned int timeout)
{
    int r = 0;
    for (;;)
    {
        r = safe_write(sockfd, buf, count, timeout);
        if (r == -1)
            if (errno == EAGAIN || errno == EINTR)
                continue;
        break;
    }
    return r;
}

/**
 * @brief attempts to read count bytes from fd into the buf
 * @param fd        Read from fd
 * @param buf       The buffer read 
 * @param count     The number of bytes read from fd
 * @param timeout   Set read timeout seconds
 * @return -2:timeout, -1:error, 0:end of file, >0:succ read number of bytes
 * 
 * 函数是系统函数'read'的封装，在用read时，用此函数时即可.
 * 如果成功，则返回读取的字节数(0表示文件结束), 如果出现错误，则返回-1，并适当设置errno.
 */
ssize_t safe_read(int fd, char *buf, size_t count, unsigned int timeout)
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
        return read(fd, buf, count);

    return -2;
}

/**
 * @brief writes up to count bytes from buf to fd
 * @param fd        Write to fd
 * @param buf       The buffer write
 * @param count     The number of bytes write
 * @param timeout   Set write timeout seconds
 * @return -2:timeout, -1:error, >0:succ write number of bytes
 * 
 * 函数是系统函数'write'的封装，在用write时，用此函数时即可.
 */
ssize_t safe_write(int fd, const char *buf, size_t count, unsigned int timeout)
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
        return write(fd, buf, count);

    return -2;
}

#ifdef _TEST
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

#endif
