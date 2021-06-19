#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "scdb.h"
#include "utils.h"

void scdb_free(struct scdb *c)
{
    if (c->map)
    {
        munmap(c->map, c->size);
        c->map = 0;
    }
}

void scdb_findstart(struct scdb *c)
{
    c->loop = 0;
}

/**
 * @brief   初始化 struct scdb 结构
 * @param c 指向 struct scdb 对象的指针
 * @param fd    cdb 文件的句柄
 */
void scdb_init(struct scdb *c, int fd)
{
    struct stat st;
    char *x;

    scdb_free(c);
    scdb_findstart(c);
    c->fd = fd;

    if (fstat(fd, &st) == 0)
    {
        if (st.st_size <= 0xffffffff)
        {
            x = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
            if (x + 1)
            {
                c->size = st.st_size;
                c->map = x;
            }
        }
    }
}

int scdb_read(struct scdb *c, char *buf, unsigned int len, uint32_t pos)
{
    if (c->map)
    {
        if ((pos > c->size) || (c->size - pos < len))
            goto FMT;
        memcpy(buf, c->map + pos, len);
    }
    else
    {
        if (lseek(c->fd, pos, SEEK_SET) == -1)
            return -1;
        while (len > 0)
        {
            int r;
            do
                r = read(c->fd, buf, len);
            while ((r == -1) && (errno == EINTR));
            if (r == -1)
                return -1;
            if (r == 0)
                goto FMT;
            buf += r;
            len -= r;
        }
    }

    return 0;

FMT:
    return -1;
}

static int match(struct scdb *c, char *key, unsigned int len, uint32_t pos)
{
    char buf[32];
    int n;

    while (len > 0)
    {
        n = sizeof(buf);
        if (n > len)
            n = len;
        if (scdb_read(c, buf, n, pos) == -1)
            return -1;
        if (strncmp(buf, key, n))
            return 0;

        pos += n;
        key += n;
        len -= n;
    }

    return 1;
}

int scdb_findnext(struct scdb *c, char *key, unsigned int len)
{
    char buf[8];
    uint32_t pos;
    uint32_t u;

    if (!c->loop)
    {
        u = scdb_hash(key, len);
        if (scdb_read(c, buf, 8, (u << 3) & 2047) == -1)
            return -1;
        uint32_unpack(buf + 4, &c->hslots);
        if (!c->hslots)
            return 0;
        uint32_unpack(buf, &c->hpos);
        c->khash = u;
        u >>= 8;
        u %= c->hslots;
        u <<= 3;
        c->kpos = c->hpos + u;
    }

    while (c->loop < c->hslots)
    {
        if (scdb_read(c, buf, 8, c->kpos) == -1)
            return -1;
        uint32_unpack(buf + 4, &pos);
        if (!pos)
            return 0;
        c->loop += 1;
        c->kpos += 8;
        if (c->kpos == c->hpos + (c->hslots << 3))
            c->kpos = c->hpos;
        uint32_unpack(buf, &u);
        if (u == c->khash)
        {
            if (scdb_read(c, buf, 8, pos) == -1)
                return -1;
            uint32_unpack(buf, &u);
            if (u == len)
                switch (match(c, key, len, pos + 8))
                {
                case -1:
                    return -1;
                case 1:
                    uint32_unpack(buf + 4, &c->dlen);
                    c->dpos = pos + 8 + len;
                    return 1;

                default:
                    break;
                }
        }
    }

    return 0;
}

int scdb_find(struct scdb *c, char *key, unsigned int len)
{
    scdb_findstart(c);
    return scdb_findnext(c, key, len);
}

/**
 * @brief   从 cdb 文件中获取指定 key 的值，没有返回 NULL
 * @param cdb_fn    指向 cdb 文件
 * @param key       去获取的 key
 * @param keylen    key 的长度
 * @return 返回与key关联的值，未找到或失败返回 NULL
 * @note 返回的 value 是在堆上分配的，应该手动释放 free()
 */
char *scdb_get_alloc(char *cdb_fn, char *key, unsigned int keylen)
{
    int fd = open(cdb_fn, O_RDONLY | O_NDELAY);
    if (fd == -1)
        return NULL;

    struct scdb c;
    scdb_init(&c, fd);

    int ret = scdb_find(&c, key, keylen);
    if (ret == 1)
    {
        unsigned int datalen = scdb_datalen(&c);
        char *data = malloc(datalen);
        if (!data)
            goto CFAIL;
        if (scdb_read(&c, data, datalen, scdb_datapos(&c)) == -1)
        {
            free(data);
            goto CFAIL;
        }

        scdb_free(&c);
        close(fd);

        return data;
    }

CFAIL:
    scdb_free(&c);
    close(fd);

    return NULL;
}

#ifdef _TEST
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

void usage(char *prog)
{
    fprintf(stderr, "Usage: %s [cdb file] [key]\n", prog);
    exit(0);
}
void main(int argc, char **argv)
{
    char *fn = argv[1];
    if (!fn)
        usage(argv[0]);
    char *key = argv[2];
    if (!key)
        usage(argv[0]);

    char *value = scdb_get_alloc(fn, key, strlen(key));
    if (value)
    {
        printf("Get key:%s -> value:%s\n", key, value);
        free(value);
    }
    else
        printf("Get key:%s -> Not Found\n", key);
}
#endif