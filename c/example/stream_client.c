/*
    a stream socket client

    gcc -g stream_client.c
    ./a.out <ip> <port>
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 100 // 一次可以接收最大字节数

// 获取 sockaddr IPv4或IPv6
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)sa)->sin_addr);
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/**
 * @brief Create socket and connect to special ip:service
 * @param ip        
 * @param service   The port or service name (see services(5))
 * @param err       The error string
 * @param err_size  The memory size of err
 * @return -1:fail, the err is error string. Other is succ
 * 
 * It return socket fd
 */
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

#ifdef _TEST
int main(int argc, char **argv)
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE] = {0};
    char err[1024] = {0};
    size_t err_size = sizeof(err);

    sockfd = s_connect(argv[1], argv[2], err, err_size);
    if (sockfd == -1)
    {
        fprintf(stderr, "%s\n", err);
        return 1;
    }

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
    {
        fprintf(stderr, "recv: %s\n", strerror(errno));
        close(sockfd);
        return 1;
    }

    buf[numbytes] = '\0';
    printf("client: received '%s'\n", buf);

    close(sockfd);

    return 0;
}
#endif
