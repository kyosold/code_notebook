#ifndef _S_BASE64_H
#define _S_BASE64_H

unsigned char *sbase64_encode_alloc(const unsigned char *, int, int *);
unsigned char *sbase64_decode_alloc(const unsigned char *, int, int *);

#endif