#ifndef S_CDB_UTILS_H
#define S_CDB_UTILS_H

#include <stdint.h>

#define SCDB_HASHSTART 5381

void uint32_pack(char s[4], uint32_t u);
void uint32_unpack(char s[4], uint32_t *u);

void uint32_pack_big(char s[4], uint32_t u);
void uint32_unpack_big(char s[4], uint32_t *u);

uint32_t scdb_hashadd(uint32_t h, unsigned char c);
uint32_t scdb_hash(char *buf, unsigned int len);

#endif
