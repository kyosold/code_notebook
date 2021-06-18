/**
 * Build:
 *  gcc -g -o scdb scdb.c buffer.c utils.c
 * 
 * Exec:
 *  ./scdb ./cfg.cdb 'email'
 */

#ifndef S_CDB_H
#define S_CDB_H

#include <stdint.h>

#define SCDB_HASHSTART 5381

struct scdb
{
    char *map; // 0 if no map is available
    int fd;
    uint32_t size;   // initialized if map is nonzero
    uint32_t loop;   // number of hash slots searched under this key
    uint32_t khash;  // initialized if loop is nonzero
    uint32_t kpos;   // initialized if loop is nonzero
    uint32_t hpos;   // initialized if loop is nonzero
    uint32_t hslots; // initialized if loop is nonzero
    uint32_t dpos;   // initialized if scdb_findnext() returns 1
    uint32_t dlen;   // initialized if scdb_findnext() returns 1
};

#define scdb_datalen(c) ((c)->dlen)
#define scdb_datapos(c) ((c)->dpos)

char *scdb_get_alloc(char *cdb_fn, char *key, unsigned int keylen);

#endif