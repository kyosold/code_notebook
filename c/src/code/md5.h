#ifndef _S_MD5_H
#define _S_MD5_H

#include <stdint.h>
#include <stdio.h>

void make_digest(char *md5str, const unsigned char *digest);
void make_digest_ex(char *md5str, const unsigned char *digest, int len);

/* MD5 context. */
typedef struct
{
    uint32_t lo, hi;
    uint32_t a, b, c, d;
    unsigned char buffer[64];
    uint32_t block[16];
} S_MD5_CTX;

void S_MD5Init(S_MD5_CTX *ctx);
void S_MD5Update(S_MD5_CTX *ctx, const void *data, size_t size);
void S_MD5Final(unsigned char *result, S_MD5_CTX *ctx);

void s_md5(const char *str, size_t strlen, int raw_output, char *result, size_t result_size);
int s_md5_file(const char *filename, int raw_output, char *result, size_t result_size);

#endif
