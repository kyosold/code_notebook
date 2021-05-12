#ifndef _S_QUOTE_PRINT_H
#define _S_QUOTE_PRINT_H

#include <stdio.h>

unsigned char *s_quoted_printable_encode(const char *str, size_t str_len, size_t *ret_length);
unsigned char *s_quoted_printable_decode(const char *str, size_t str_len, size_t *ret_length);

#endif