/*
 * @Author: songjian <kyosold@qq.com>
 * @Date: 2022-08-11 11:29:32
 * @LastEditors: kyosold kyosold@qq.com
 * @LastEditTime: 2022-08-18 16:56:44
 * @FilePath: /socket/socket_io.h
 * @Description:
 *
 * Copyright (c) 2022 by kyosold kyosold@qq.com, All Rights Reserved.
 */
#ifndef _SOCKET_IO_H_
#define _SOCKET_IO_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

char *get_socket_error_info();

char *get_remote_host(struct sockaddr_storage *addr);
char *get_remote_port(struct sockaddr_storage *addr);

/***********************************************************
 * Socket 相关操作
 **********************************************************/
int ndelay_on(int fd);
int ndelay_off(int fd);

int socket_connect(char *ip, char *service, int timeout);
int socket_listen(char *service, int backlog);

ssize_t socket_read(int socket_fd, char *buf, size_t buf_size, int timeout);
ssize_t socket_write(int socket_fd, char *buf, size_t buf_len, int timeout);

ssize_t safe_read(int fd, char *buf, size_t buf_size, unsigned int timeout);
ssize_t safe_write(int fd, char *buf, size_t buf_len, unsigned int timeout);

void *get_in_addr(struct sockaddr *sa);

/***********************************************************
 * Socket SSL 相关操作
 **********************************************************/
#ifdef ssl_enable
#include <openssl/ssl.h>

#define SSL_SOCKET_VERIFY_NONE 0 // 不做验证
#define SSL_SOCKET_VERIFY 1      // 做验证

#define CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"

SSL_CTX *ssl_socket_new_ctx(int method);
int load_ca_cert(SSL_CTX *ctx, char *ca_file, char *ca_path);
int load_certificates(SSL_CTX *ctx, char *cert_file, char *key_file, char *password);
void ssl_socket_set_verify_certificates(SSL_CTX *ctx, int bit, int depth);
int ssl_socket_set_cipher_list_serv(SSL_CTX *ctx, const char *ciphers);
int ssl_socket_set_cipher_file_serv(SSL_CTX *ctx, char *cipher_file);
void ssl_socket_set_tmp_dh_path(SSL_CTX *ctx, char *dh_path);
void ssl_socket_free_ctx(SSL_CTX *ctx);

SSL *ssl_socket_connect(SSL_CTX *ctx, int fd, int timeout);
SSL *ssl_socket_accept(SSL_CTX *ctx, int fd, int timeout);
void ssl_socket_cleanup(SSL *ssl);

int ssl_socket_read(SSL *ssl, int fd, char *buf, size_t buf_size, int timeout);
int ssl_socket_write(SSL *ssl, int fd, char *buf, size_t buf_len, int timeout);

void show_certs(SSL *ssl);

#endif

#endif