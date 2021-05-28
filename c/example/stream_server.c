/*
    stream socket server

    gcc -g stream_server.c
    ./a.out <port>
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490" // 提供给使用者连接的 port
#define BACKLOG 10  // 有多少个特定的连接队列 (pending connections queue)

void sigchld_cb(int s)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

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

/**
 * @brief Create, Bind and listen socket fd
 * @param service   The port for listen or service name (see services(5))
 * @param err       The error string
 * @param err_size  The memory size of err
 * @return -1:fail, the err is error string. Other is succ
 * 
 * It return socket fd
 */
int s_socket(char *service, char *err, size_t err_size)
{
    int sockfd;
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
 * @param sockfd    listened socket fd
 * @param rip       remote ip
 * @param rip_size  the memory size of rip
 * @param rport     remote port
 * @param rport_size the meory size of rport
 * @param err       the error string
 * @param err_size  the memory size of err
 * @return -1:fail, the err is error string. Other is succ
 * 
 * It return accepted client socket fd
 */
int s_accept(int sockfd, char *rip, size_t rip_size, char *rport, size_t rport_size, char *err, size_t err_size)
{
    int new_fd;
    struct sockaddr_storage their_addr; // 连接者的地址信息
    socklen_t sin_size = sizeof(their_addr);
    char s[INET6_ADDRSTRLEN] = {0};
    // int p = 0;

    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1)
    {
        snprintf(err, err_size, "accept: %s", strerror(errno));
        return -1;
    }

    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof(s));
    // p = ntohs(get_in_port((struct sockaddr *)&their_addr));

    snprintf(rip, rip_size, "%s", s);
    snprintf(rport, rport_size, "%d", ntohs(get_in_port((struct sockaddr *)&their_addr)));

    return new_fd;
}

#ifdef _TEST
int main(int argc, char **argv)
{
    int sockfd, new_fd; // 在 sockfd 进行 listen, new_fd 是新的连接

    struct sigaction sa;
    char err[1024] = {0};
    size_t err_size = sizeof(err);
    char rip[INET6_ADDRSTRLEN] = {0};
    char rport[128] = {0};

    sockfd = s_socket(argv[1], err, err_size);
    if (sockfd == -1)
    {
        fprintf(stderr, "%s\n", err);
        return 1;
    }

    sa.sa_handler = sigchld_cb; // 收拾全部死掉的子进程
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        fprintf(stderr, "sigaction SIGCHLD fail: %s\n", strerror(errno));
        close(sockfd);
        return 1;
    }

    printf("server: waiting for connections...\n");
    while (1)
    {
        // 主要的 accept 循环
        new_fd = s_accept(sockfd,
                          rip, sizeof(rip),
                          rport, sizeof(rport),
                          err, err_size);
        if (new_fd == -1)
        {
            fprintf(stderr, "%s\n", err);
            continue;
        }
        printf("server: got connection from %s:%s\n", rip, rport);

        if (!fork())
        {
            // 这个是 child process
            close(sockfd); // child 不需要 listener

            if (send(new_fd, "Hello, world!", 13, 0) == -1)
                fprintf(stderr, "send: %s\n", strerror(errno));

            close(new_fd);

            exit(0);
        }

        close(new_fd); // parent 不需要这个
    }

    return 0;
}
#endif
