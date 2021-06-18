#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "buffer.h"

void buffer_init(buffer *s, int (*op)(), int fd, char *buf, unsigned int len)
{
    s->x = buf;
    s->fd = fd;
    s->op = op;
    s->p = 0;
    s->n = len;
}

static int allwrite(int (*op)(), int fd, char *buf, unsigned int len)
{
    int w;

    while (len)
    {
        w = op(fd, buf, len);
        if (w == -1)
        {
            if (errno == EINTR)
                continue;
            return -1; // note that some data may have been written
        }
        if (w == 0)
            ; // luser's fault
        buf += w;
        len -= w;
    }
    return 0;
}

int buffer_flush(buffer *s)
{
    int p;
    p = s->p;
    if (!p)
        return 0;
    s->p = 0;
    return allwrite(s->op, s->fd, s->x, p);
}

int buffer_putalign(buffer *s, char *buf, unsigned int len)
{
    unsigned int n;

    while (len > (n = s->n - s->p))
    {
        memcpy(s->x + s->p, buf, n);
        s->p += n;
        buf += n;
        len -= n;
        if (buffer_flush(s) == -1)
            return -1;
    }
    // now len <= s->n - s->p
    memcpy(s->x + s->p, buf, len);
    s->p += len;
    return 0;
}

int buffer_putflush(buffer *s, char *buf, unsigned int len)
{
    if (buffer_flush(s) == -1)
        return -1;

    return allwrite(s->op, s->fd, buf, len);
}
