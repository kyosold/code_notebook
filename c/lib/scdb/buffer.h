#ifndef S_CDB_BUFFER_H
#define S_CDB_BUFFER_H

typedef struct buffer
{
    char *x;
    unsigned int p;
    unsigned int n;
    int fd;
    int (*op)();
} buffer;

void buffer_init(buffer *s, int (*op)(), int fd, char *buf, unsigned int len);
int buffer_putalign(buffer *s, char *buf, unsigned int len);
int buffer_flush(buffer *s);
int buffer_putflush(buffer *s, char *buf, unsigned int len);

#endif