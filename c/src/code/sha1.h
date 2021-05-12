#ifndef _S_SHA1_H
#define _S_SHA1_H
#include <stdint.h>

typedef struct
{
    uint32_t state[5];        /* state (ABCD) */
    uint32_t count[2];        /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64]; /* input buffer */
} S_SHA1_CTX;

void S_SHA1Init(S_SHA1_CTX *);
void S_SHA1Update(S_SHA1_CTX *, const unsigned char *, unsigned int);
void S_SHA1Final(unsigned char[20], S_SHA1_CTX *);
void make_sha1_digest(char *sha1str, unsigned char *digest);

void s_sha1(const char *str, size_t strlen, int raw_output, char *result, size_t result_size);
int s_sha1_file(const char *filename, int raw_output, char *result, size_t result_size);

#endif
