#ifndef _S_ICONV_H
#define _S_ICONV_H

char *s_iconv_string_alloc(const char *in_p, size_t in_len,
                           size_t *out_len,
                           const char *out_charset, const char *in_charset,
                           char *err_str, size_t err_str_size);

#endif