/*
------------------------------------------------------------------
make ssl server: 
    1. create socket, bind and listen it
    2. create ssl_ctx with cert and key: sock_ssl_ctx_new();

    3.      accept a client fd
    4.      create ssl for the client fd: ssl = sock_ssl_new_client();
    5.      read and write with security socket: sock_ssl_read(), sock_ssl_write()
    6.      close client: 
                a. sock_ssl_free_client(ssl);
                b. close(client_fd);

    7. close listen fd: close(listen_fd);
    8. cleanup ssl: sock_ssl_ctx_free(ctx);

    **********************************************
    listenfd = socket_bind_listen(port);
    ctx = sock_ssl_ctx_new(crt, key);
    while(1) {
        // accept
        cfd = accept(listenfd, ...);
        ssl = sock_ssl_new_client(cfd);

        sock_ssl_read(ssl, buf, ...);
        sock_ssl_write(ssl, buf, ...);

        sock_ssl_free_client(ssl);
        close(cfd);
    }
    close(listenfd);
    sock_ssl_ctx_free(ctx);
    **********************************************

Running the program requires that a SSL certificate and private key:
    1. Generate the CA private key and certificate:

        openssl genrsa -out ca_private.key 2048
        openssl req -x509 -new -nodes -key ca_private.key -out ca_private.pem -days 365

    2. Generate the private key and certificate to be signed

        openssl genrsa -out serv_private.key 2048
        openssl req -new -key serv_private.key -out serv_private.csr

    3. Sign the server certificate using the CA certificate and private key:

        openssl x509 -req -in serv_private.csr -CA ca_private.pem -CAkey ca_private.key -CAcreateserial -out serv_private.crt -days 365
------------------------------------------------------------------

make ssl client:
    1. create ssl_ctx with cert and key: ctx = sock_ssl_ctx_new();
    2. connect to remote server: fd = s_connect();
    3. with tls/ssl server handshake: ssl = sock_ssl_connect()
    4. read and write with security socket: sock_ssl_read(), sock_ssl_write()
    5. close socket and cleanup ssl:
        a. close(sockfd);
        b. sock_ssl_cleanup(ssl);
        c. sock_ssl_ctx_free(ctx);

*/

#ifndef _S_SIO_SSL_H
#define _S_SIO_SSL_H

#include <openssl/ssl.h>

int check_certs(SSL *ssl);

SSL_CTX *sock_ssl_ctx_new(int type, char *crtfile, char *keyfile, char *err, size_t err_size);
void sock_ssl_ctx_free(SSL_CTX *ctx);

SSL *sock_ssl_new_client(SSL_CTX *ctx, int fd, char *err, size_t err_size);
SSL *sock_ssl_connect(SSL_CTX *ctx, int fd, char *err, size_t err_size);
void sock_ssl_cleanup(SSL *ssl);

int sock_ssl_read(SSL *ssl, int fd, char *buf, size_t buf_size, int timeout, char *err, size_t err_size);
int sock_ssl_write(SSL *ssl, int fd, char *buf, size_t buf_size, int timeout, char *err, size_t err_size);

#endif
