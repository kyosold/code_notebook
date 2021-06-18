/**
 * scdb 是一个快速、可靠、简单的包，用于创建和读取常量数据库。它的数据库结构提供了几个特点：
 *   - 快速查找：在大型数据库中成功查找通常只需要两次磁盘访问。不成功的查找只需要一个。
 *   - 低开销：数据库使用 2048 字节，加上每条记录 24 字节，加上键和数据的空间。
 *   - 没有随机限制： cdb 可以处理最大 4 GB 的任何数据库。没有其他限制；记录甚至不必放入内存。数据库以与机器无关的格式存储。
 *   - 快速原子数据库替换： cdbmake 可以比其他散列包快两个数量级地重写整个数据库。
 *   - 快速数据库转储： cdbdump 以与 cdbmake 兼容的格式打印数据库的内容。
 * cdb 旨在用于电子邮件等关键任务应用程序。数据库替换对于系统崩溃是安全的。读者不必在重写期间暂停。
 * 
 * Build:
 *  gcc -g -o scdb_make scdb_make.c buffer.c utils.c
 * 
 * Exec:
 *  ./scdb_make 'cfg.ini' '=' 'cfg.cdb'
 * 
 * cfg.ini:
 *      version=1.0
 *      name=cdb_test
 *      email=kyosold@qq.com
 * 
 * 
 */

#ifndef S_CDB_MAKE_H
#define S_CDB_MAKE_H

#include <stdint.h>
#include "buffer.h"

#define SCDB_HPLIST 1000

struct scdb_hp
{
    uint32_t h;
    uint32_t p;
};

struct scdb_hplist
{
    struct scdb_hp hp[SCDB_HPLIST];
    struct scdb_hplist *next;
    int num;
};

struct scdb_make
{
    char bspace[8192];
    char final[2048];
    uint32_t count[256];
    uint32_t start[256];
    struct scdb_hplist *head;
    struct scdb_hp *split; // includes space for hash
    struct scdb_hp *hash;
    uint32_t numentries;
    buffer b;
    uint32_t pos;
    int fd;
};

int scdb_make_start(struct scdb_make *c, int fd);
int scdb_make_addbegin(struct scdb_make *c, unsigned int keylen, unsigned int datalen);
int scdb_make_addend(struct scdb_make *c, unsigned int keylen, unsigned int datalen, uint32_t h);
int scdb_make_add(struct scdb_make *c,
                  char *key, unsigned int keylen,
                  char *data, unsigned int datalen);
int scdb_make_finish(struct scdb_make *c);

int scdb_make_gen_file(char *fn, char sep, char *cdb_fn);

#endif