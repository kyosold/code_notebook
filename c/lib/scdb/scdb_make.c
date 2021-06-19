#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include "scdb_make.h"
#include "utils.h"

// -------------------------------------------------

int scdb_make_start(struct scdb_make *c, int fd)
{
    c->head = 0;
    c->split = 0;
    c->hash = 0;
    c->numentries = 0;
    c->fd = fd;
    c->pos = sizeof(c->final);

    buffer_init(&c->b, write, fd, c->bspace, sizeof c->bspace);

    if (lseek(fd, (off_t)c->pos, SEEK_SET) == -1)
        return -1;
    else
        return 0;
}

static int posplus(struct scdb_make *c, uint32_t len)
{
    uint32_t newpos = c->pos + len;
    if (newpos < len)
        return -1;
    c->pos = newpos;
    return 0;
}

int scdb_make_addend(struct scdb_make *c, unsigned int keylen, unsigned int datalen, uint32_t h)
{
    struct scdb_hplist *head;
    head = c->head;
    if (!head || (head->num >= SCDB_HPLIST))
    {
        head = (struct scdb_hplist *)malloc(sizeof(struct scdb_hplist));
        if (!head)
            return -1;
        head->num = 0;
        head->next = c->head;
        c->head = head;
    }
    head->hp[head->num].h = h;
    head->hp[head->num].p = c->pos;
    ++head->num;
    ++c->numentries;
    if (posplus(c, 8) == -1)
        return -1;
    if (posplus(c, keylen) == -1)
        return -1;
    if (posplus(c, datalen) == -1)
        return -1;
    return 0;
}

int scdb_make_addbegin(struct scdb_make *c, unsigned int keylen, unsigned int datalen)
{
    char buf[8];
    if (keylen > 0xffffffff)
        return -1;
    if (datalen > 0xffffffff)
        return -1;

    uint32_pack(buf, keylen);
    uint32_pack(buf + 4, datalen);
    if (buffer_putalign(&c->b, buf, 8) == -1)
        return -1;
    return 0;
}

int scdb_make_add(struct scdb_make *c,
                  char *key, unsigned int keylen,
                  char *data, unsigned int datalen)
{
    if (scdb_make_addbegin(c, keylen, datalen) == -1)
        return -1;
    if (buffer_putalign(&c->b, key, keylen) == -1)
        return -1;
    if (buffer_putalign(&c->b, data, datalen) == -1)
        return -1;
    return scdb_make_addend(c, keylen, datalen, scdb_hash(key, keylen));
}

int scdb_make_finish(struct scdb_make *c)
{
    char buf[8];
    int i;
    uint32_t len;
    uint32_t u;
    uint32_t memsize;
    uint32_t count;
    uint32_t where;
    struct scdb_hplist *x;
    struct scdb_hp *hp;

    for (i = 0; i < 256; ++i)
        c->count[i] = 0;

    for (x = c->head; x; x = x->next)
    {
        i = x->num;
        while (i--)
            ++c->count[255 & x->hp[i].h];
    }

    memsize = 1;
    for (i = 0; i < 256; ++i)
    {
        u = c->count[i] * 2;
        if (u > memsize)
            memsize = u;
    }

    memsize += c->numentries; // no overflow possible up to now
    u = (uint32_t)0 - (uint32_t)1;
    u /= sizeof(struct scdb_hp);
    if (memsize > u)
        return -1;

    c->split = (struct scdb_hp *)malloc(memsize * sizeof(struct scdb_hp));
    if (!c->split)
        return -1;

    c->hash = c->split + c->numentries;

    u = 0;
    for (i = 0; i < 256; ++i)
    {
        u += c->count[i]; // bounded by numentries, so no overflow
        c->start[i] = u;
    }

    for (x = c->head; x; x = x->next)
    {
        i = x->num;
        while (i--)
            c->split[--c->start[255 & x->hp[i].h]] = x->hp[i];
    }

    for (i = 0; i < 256; ++i)
    {
        count = c->count[i];

        len = count + count; // no overflow possible
        uint32_pack(c->final + 8 * i, c->pos);
        uint32_pack(c->final + 8 * i + 4, len);

        for (u = 0; u < len; ++u)
            c->hash[u].h = c->hash[u].p = 0;

        hp = c->split + c->start[i];
        for (u = 0; u < count; ++u)
        {
            where = (hp->h >> 8) % len;
            while (c->hash[where].p)
                if (++where == len)
                    where = 0;
            c->hash[where] = *hp++;
        }

        for (u = 0; u < len; ++u)
        {
            uint32_pack(buf, c->hash[u].h);
            uint32_pack(buf + 4, c->hash[u].p);
            if (buffer_putalign(&c->b, buf, 8) == -1)
                return -1;
            if (posplus(c, 8) == -1)
                return -1;
        }
    }

    if (buffer_flush(&c->b) == -1)
        return -1;
    if (lseek(c->fd, 0, SEEK_SET) == -1)
        return -1;

    return buffer_putflush(&c->b, c->final, sizeof(c->final));
}

/**
 * @brief   把文件 fn 生成对应的 cdb 文件
 * @param fn        需要被生成的文件
 * @param sep       文件内每行按什么分隔区分key与val, (name=value, sep is =)
 * @param cdb_fn    生成的 cdb 文件名
 * @return -1:fail, 返回生成的行数.
 */
int scdb_make_gen_file(char *fn, char sep, char *cdb_fn)
{
    struct scdb_make c;
    char cdb_fn_temp[1024] = {0};
    char *tok = NULL;
    int fd = -1;
    FILE *fp = NULL;
    int num = 0;

    fp = fopen(fn, "r");
    if (!fp)
        return -1;

    snprintf(cdb_fn_temp, sizeof(cdb_fn_temp), "%s.tmp", cdb_fn);
    fd = open(cdb_fn_temp, O_WRONLY | O_NDELAY | O_TRUNC | O_CREAT, 0644);
    if (fd == -1)
        goto SFAIL;
    if (scdb_make_start(&c, fd) == -1)
        goto SFAIL;

    char buf[4096] = {0};
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        tok = (char *)memchr(buf, sep, strlen(buf));
        if (!tok)
            continue;

        *tok = '\0';

        if (scdb_make_add(&c, buf, strlen(buf), tok + 1, strlen(tok + 1)) == -1)
            goto SFAIL;

        num++;
    }

    if (!feof(fp))
        goto SFAIL;

    if (scdb_make_finish(&c) == -1)
        goto SFAIL;
    if (fsync(fd) == -1)
        goto SFAIL;

    if (rename(cdb_fn_temp, cdb_fn))
        goto SFAIL;

    fclose(fp);
    close(fd);

    return num;

SFAIL:
    if (fp != NULL)
        fclose(fp);
    if (fd != -1)
        close(fd);

    return -1;
}

#ifdef _TEST
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

void usage(char *prog)
{
    fprintf(stderr, "Usage: %s [in.ini] [sep] [out.cdb]\n", prog);
    exit(0);
}
void main(int argc, char **argv)
{
    char *fn = argv[1];
    if (!fn)
        usage(argv[0]);
    char *sep = argv[2];
    if (!sep)
        usage(argv[0]);
    char *cdb_fn = argv[2];
    if (!cdb_fn)
        usage(argv[0]);

    int ret = scdb_make_gen_file(fn, *sep, cdb_fn);
    if (ret == 0)
        printf("Succ.\n");
    else
        printf("Fail\n");
}
#endif