/*
 * @Author: songjian <kyosold@qq.com>
 * @Date: 2022-08-17 16:02:34
 * @LastEditors: kyosold kyosold@qq.com
 * @LastEditTime: 2022-08-17 16:48:01
 * @FilePath: /socket/example.c
 * @Description:
 *
 * Copyright (c) 2022 by kyosold kyosold@qq.com, All Rights Reserved.
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
#include <fcntl.h>
#include <sys/time.h>
#include "socket_io.h"

int process_child(int conn_fd, int timeout)
{
    char buf[1024] = {0};
    int n = 0;

    // greeting
    snprintf(buf, sizeof(buf), "220 greeting\r\n");
    n = safe_write(conn_fd, buf, strlen(buf), timeout);
    printf("write to remote: [%d]:%s\n", n, buf);

    for (;;)
    {
        n = socket_read(conn_fd, buf, sizeof(buf), timeout);
        if (n == -1)
        {
            if (errno == ETIMEDOUT)
            {
                printf("read from remote timeout:%d\n", timeout);
                continue;
            }
            printf("socket_read fail: %s\n", get_socket_error_info());
            return -1;
        }
        else if (n == 0)
        {
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

void usage(char *prog)
{
    printf("Usage:\n");
    printf("  1. client connect:\n");
    printf("    %s conn [ip] [port] [timeout]\n", prog);
    printf("  2. server listen:\n");
    printf("    %s listen [port] [backlog] [timeout]\n", prog);
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        usage(argv[0]);
        return 1;
    }

    if (strcasecmp(argv[1], "conn") == 0)
    {
        if (argc != 5)
        {
            usage(argv[0]);
            return 1;
        }
        char *ip = argv[2];
        char *port = argv[3];
        int timeout = atoi(argv[4]);

        /*******************************************
         * 1. Connect to server
         ******************************************/
        int socket_fd = socket_connect(ip, port, timeout);
        if (socket_fd == -1)
        {
            printf("socket_connect to %s:%s fail: %s\n", ip, port, get_socket_error_info());
            return 1;
        }

        /*******************************************
         * 2. Read/Write with server
         ******************************************/
        process_child(socket_fd, timeout);

        /*******************************************
         * 3. Disconnect with server
         ******************************************/
        close(socket_fd);
    }
    else if (strcasecmp(argv[1], "listen") == 0)
    {
        if (argc != 5)
        {
            usage(argv[0]);
            return 1;
        }

        char *port = argv[2];
        int backlog = atoi(argv[3]);
        int timeout = atoi(argv[4]);

        /*******************************************
         * 1. Listen port
         ******************************************/
        int listen_fd = socket_listen(port, backlog);
        if (listen_fd == -1)
        {
            printf("socket_listen fail: %s\n", get_socket_error_info());
            return 1;
        }

        /********************************************
         * 2. accept client
         *******************************************/
        int conn_fd;
        struct sockaddr_storage client_addr;
        socklen_t sin_size = sizeof(client_addr);
        char *client_ip;
        while (1)
        {
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
            if (child_pid == 0)
            {
                close(listen_fd);

                /********************************************
                 * 3. read/write with client
                 *******************************************/
                int n = process_child(conn_fd, timeout);
                printf("child exit(%d)\n", n);
            }
            close(conn_fd);
        }
    }

    return 0;
}