/*
*   gcc -g -o epoll_server epoll_server.c
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <libgen.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <getopt.h>

#define MAX_EVENT_NUMBER 1024 //event
#define BACKLOG 20
#define BUFFER_SIZE 10 //Buffer Size
#define ENABLE_ET 1    //Enable ET mode

/* Set file descriptor to non-congested  */
int set_nonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/* Register EPOLLIN on file descriptor FD into the epoll kernel event table indicated by epoll_fd, and the parameter enable_et specifies whether et mode is enabled for FD */
void add_fd(int epoll_fd, int fd, bool enable_et)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN; //Registering the fd is readable
    if (enable_et)
    {
        event.events |= EPOLLET;
    }

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event); //Register the fd with the epoll kernel event table
    set_nonblocking(fd);
}

/*  LT Work mode features: robust but inefficient */
void lt_process(struct epoll_event *events, int number, int epoll_fd, int listen_fd)
{
    char buf[BUFFER_SIZE];
    int i;
    for (i = 0; i < number; i++) //number: number of events ready
    {
        int sockfd = events[i].data.fd;
        if (sockfd == listen_fd) //If it is a file descriptor for listen, it indicates that a new customer is connected to
        {
            struct sockaddr_in client_address;
            socklen_t client_addrlength = sizeof(client_address);
            int connfd = accept(listen_fd, (struct sockaddr *)&client_address, &client_addrlength);
            add_fd(epoll_fd, connfd, false); //Register new customer connection fd to epoll event table, using lt mode
        }
        else if (events[i].events & EPOLLIN) //Readable with client data
        {
            // This code is triggered as long as the data in the buffer has not been read.This is what LT mode is all about: repeating notifications until processing is complete
            printf("lt mode: event trigger once!\n");
            memset(buf, 0, BUFFER_SIZE);
            int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
            if (ret <= 0) //After reading the data, remember to turn off fd
            {
                close(sockfd);
                continue;
            }
            printf("get %d bytes of content: %s\n", ret, buf);
        }
        else
        {
            printf("something unexpected happened!\n");
        }
    }
}

/* ET Work mode features: efficient but potentially dangerous */
void et_process(struct epoll_event *events, int number, int epoll_fd, int listen_fd)
{
    char buf[BUFFER_SIZE];
    int i;
    for (i = 0; i < number; i++)
    {
        int sockfd = events[i].data.fd;
        if (sockfd == listen_fd)
        {
            struct sockaddr_in client_address;
            socklen_t client_addrlength = sizeof(client_address);
            int connfd = accept(listen_fd, (struct sockaddr *)&client_address, &client_addrlength);
            add_fd(epoll_fd, connfd, true); //Use et mode
        }
        else if (events[i].events & EPOLLIN)
        {
            /* This code will not be triggered repeatedly, so we cycle through the data to make sure that all the data in the socket read cache is read out.This is how we eliminate the potential dangers of the ET model */

            printf("et mode: event trigger once!\n");
            while (1)
            {
                memset(buf, 0, BUFFER_SIZE);
                //int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
                int ret = read(sockfd, buf, BUFFER_SIZE);
                if (ret < 0)
                {
                    /* For non-congested IO, the following condition is true to indicate that the data has been read completely, after which epoll can trigger the EPOLLIN event on sockfd again to drive the next read operation */

                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        // 本次读取完毕
                        printf("read later!\n");
                        break;
                    }

                    close(sockfd);
                    break;
                }
                else if (ret == 0)
                {
                    printf("client disconnect\n");
                    close(sockfd);
                    break;
                }
                else //Not finished, continue reading in a loop
                {
                    printf("get %d bytes of content: %s\n", ret, buf);
                    // need append string from buf
                }
            }

            char sbuf[1024] = "250 OK\r\n";
            write(sockfd, sbuf, strlen(sbuf));
        }
        else
        {
            printf("something unexpected happened!\n");
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("usage:  %s [port]\n", argv[0]);
        return -1;
    }

    char *port = argv[1];

    char bind_host[NI_MAXHOST];
    char bind_port[NI_MAXSERV];

    int listen_fd, connfd, n;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage client_addr;
    int on = 1;
    int rv = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
    {
        printf("getaddrinfo fail:%s\n", gai_strerror(rv));
        return -1;
    }

    // 轮询找出全部结果，并 bind 到第一个能用的结果
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if (rv = getnameinfo(p->ai_addr, p->ai_addrlen,
                             bind_host, sizeof(bind_host),
                             bind_port, sizeof(bind_port),
                             NI_NUMERICHOST | NI_NUMERICSERV) != 0)
        {
            printf("getnameinfo fail:%s\n", gai_strerror(rv));
            continue;
        }

        if ((listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            printf("create socket fail(%d):%s\n", errno, strerror(errno));
            continue;
        }

        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
        {
            printf("setsockopt SO_REUSEADDR fail(%d):%s\n", errno, strerror(errno));
            close(listen_fd);
            continue;
        }

        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(listen_fd);
            printf("bind socket fail(%d):%s\n", errno, strerror(errno));
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        printf("server: failed to bind\n");
        return -1;
    }

    freeaddrinfo(servinfo); // 全部都用这个structure

    if (listen(listen_fd, BACKLOG) == -1)
    {
        printf("listen fail(%d):%s\n", errno, strerror(errno));
        return -1;
    }
    printf("Listen: %s:%s\n", bind_host, bind_port);

    struct epoll_event events[MAX_EVENT_NUMBER];
    int epoll_fd = epoll_create(5); //Event table size 5
    if (epoll_fd == -1)
    {
        printf("fail to create epoll!\n");
        return -1;
    }

    add_fd(epoll_fd, listen_fd, true); //Add listen file descriptor to event table using ET mode epoll

    while (1)
    {
        int ret = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
        if (ret < 0)
        {
            printf("epoll failure!\n");
            break;
        }

        if (ENABLE_ET)
        {
            et_process(events, ret, epoll_fd, listen_fd);
        }
        else
        {
            lt_process(events, ret, epoll_fd, listen_fd);
        }
    }

    close(listen_fd);
    return 0;
}
