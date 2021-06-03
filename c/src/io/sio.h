#ifndef _S_IO_H
#define _S_IO_H

#include <sys/types.h>
#include <sys/socket.h>

int sock_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen, unsigned int timeout);
ssize_t sock_read(int sockfd, char *buf, unsigned int count, unsigned int timeout);
ssize_t sock_write(int sockfd, char *buf, unsigned int count, unsigned int timeout);

ssize_t safe_read(int fd, char *buf, size_t count, unsigned int timeout);
ssize_t safe_write(int fd, const char *buf, size_t count, unsigned int timeout);

#endif
