/*
 * @Author: songjian <kyosold@qq.com>
 * @Date: 2022-08-15 15:24:14
 * @LastEditors: kyosold kyosold@qq.com
 * @LastEditTime: 2022-08-17 17:21:28
 * @FilePath: /socket/example_ssl.c
 * @Description:
 *
 * Copyright (c) 2022 by kyosold kyosold@qq.com, All Rights Reserved.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>

#include "socket_io.h"

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

void usage(char *prog)
{
    printf("Usage:\n");
    printf(" 1.client ssl connect:\n");
    printf("    %s conn [ip] [port]\n", prog);
    printf(" 2.server ssl listen:\n");
    printf("    %s listen [port] [pem] [private key]\n", prog);
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
        if (argc != 4)
        {
            usage(argv[0]);
            return 1;
        }
        char *ip = argv[2];
        char *port = argv[3];

        int ret = 0;
        SSL_CTX *ctx = NULL;
        SSL *ssl = NULL;

        /*******************************************
         * 1. Connect to server with non_ssl
         ******************************************/
        int socket_fd = socket_connect(ip, port, 3);
        if (socket_fd == -1)
        {
            printf("socket_connect to %s:%s fail: %s\n", ip, port, get_socket_error_info());
            return 1;
        }

        /********************************************
         * 2. 对 socket fd 做加密
         *******************************************/
        // 2.1 初始化ssl ctx
        ctx = ssl_socket_new_ctx(1);
        if (ctx == NULL)
        {
            printf("ssl_socket_new_ctx fail: %s\n", get_socket_error_info());
            return 1;
        }
        // 2.2 设置证书验证方式
        ssl_socket_set_verify_certificates(ctx, SSL_SOCKET_VERIFY_ALL, 10);
        // 2.2 使用ssl连接
        ssl = ssl_socket_connect(ctx, socket_fd, 3);
        if (ssl == NULL)
        {
            ssl_socket_free_ctx(ctx);
            printf("ssl_socket_free_ctx fail: %s\n", get_socket_error_info());
            return 1;
        }
        // 3.4 show certificates
        show_certs(ssl);
        // 2.3 read
        char buf[1024] = {0};
        int n = ssl_socket_read(ssl, socket_fd, buf, sizeof(buf), 3);
        printf("read[%d]: %s\n", n, buf);
        if (n <= 0)
        {
            printf("ssl_socket_read fail: %s\n", get_socket_error_info());

            ssl_socket_free_ctx(ctx);
            ssl_socket_cleanup(ssl);

            close(socket_fd);
            return 1;
        }
        // 2.4 write
        snprintf(buf, sizeof(buf), "HELO sina.net\r\n");
        n = ssl_socket_write(ssl, socket_fd, buf, strlen(buf), 3);
        printf("write[%d]: %s\n", n, buf);

        // 2.4 read,write
        n = ssl_socket_read(ssl, socket_fd, buf, sizeof(buf), 3);
        printf("read[%d]: %s\n", n, buf);
        snprintf(buf, sizeof(buf), "QUIT\r\n");
        n = ssl_socket_write(ssl, socket_fd, buf, strlen(buf), 3);
        printf("write[%d]: %s\n", n, buf);
        // 2.5 close
        ssl_socket_free_ctx(ctx);
        ssl_socket_cleanup(ssl);
        close(socket_fd);
        return 0;
    }
    else if (strcasecmp(argv[1], "listen") == 0)
    {
        if (argc != 5)
        {
            usage(argv[0]);
            return 1;
        }

        char *port = argv[2];
        char *cert_file = argv[3];
        char *key_file = argv[4];
        int backlog = 10;

        SSL_CTX *ctx = NULL;
        SSL *ssl = NULL;
        int ret = 0;

        /*******************************************
         * 1. Listen port with non_ssl
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
            {
                continue;
            }

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

                int n = process_child(conn_fd, cert_file, key_file);
                printf("child exit(%d)\n", n);
            }
            close(conn_fd);
        }
    }

    return 0;
}