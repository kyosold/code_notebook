#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include "quote_print.h"

#define S_QPRINT_MAXL 75

/**
 * @brief Convert a quoted-printable string to an 8 bit string
 * @param str The input string.
 * @param str_len string length
 * @param ret_length result length
 * @return Returns the 8-bit binary string. Fail return NULL
 */
unsigned char *s_quoted_printable_decode_alloc(const char *str, size_t str_len, size_t *ret_length)
{
    register unsigned int i;
    register unsigned const char *p1;
    register unsigned char *p2;
    register unsigned int h_nbl, l_nbl;

    size_t decoded_len, buf_size;
    unsigned char *retval;
    int replace_us_by_ws = 0;

    static unsigned int hexval_tbl[256] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 32, 16, 64, 64, 16, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        32, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 64, 64, 64, 64, 64, 64,
        64, 10, 11, 12, 13, 14, 15, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 10, 11, 12, 13, 14, 15, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64};

    if (replace_us_by_ws)
    {
        replace_us_by_ws = '_';
    }

    i = str_len, p1 = str;
    buf_size = str_len;

    while (i > 1 && *p1 != '\0')
    {
        if (*p1 == '=')
        {
            buf_size -= 2;
            p1++;
            i--;
        }
        p1++;
        i--;
    }

    retval = malloc(buf_size + 1);
    if (retval == NULL)
        return NULL;

    i = str_len;
    p1 = str;
    p2 = retval;
    decoded_len = 0;

    while (i > 0 && *p1 != '\0')
    {
        if (*p1 == '=')
        {
            i--, p1++;
            if (i == 0 || *p1 == '\0')
            {
                break;
            }
            h_nbl = hexval_tbl[*p1];
            if (h_nbl < 16)
            {
                /* next char should be a hexadecimal digit */
                if ((--i) == 0 || (l_nbl = hexval_tbl[*(++p1)]) >= 16)
                {
                    free(retval);
                    return NULL;
                }
                *(p2++) = (h_nbl << 4) | l_nbl, decoded_len++;
                i--, p1++;
            }
            else if (h_nbl < 64)
            {
                /* soft line break */
                while (h_nbl == 32)
                {
                    if (--i == 0 || (h_nbl = hexval_tbl[*(++p1)]) == 64)
                    {
                        free(retval);
                        return NULL;
                    }
                }
                if (p1[0] == '\r' && i >= 2 && p1[1] == '\n')
                {
                    i--, p1++;
                }
                i--, p1++;
            }
            else
            {
                free(retval);
                return NULL;
            }
        }
        else
        {
            *(p2++) = (replace_us_by_ws == *p1 ? '\x20' : *p1);
            i--, p1++, decoded_len++;
        }
    }

    *p2 = '\0';
    *ret_length = decoded_len;
    return retval;
}

/**
 * @brief Convert a 8 bit string to a quoted-printable string
 * @param str The input string.
 * @param str_len string length
 * @param ret_length result length
 * @return Returns the encoded string, to be freed with free(). Fail return NULL
 */
unsigned char *s_quoted_printable_encode_alloc(const char *str, size_t str_len, size_t *ret_length)
{
    unsigned long lp = 0;
    unsigned char c, *ret, *d;
    char *hex = "0123456789ABCDEF";

    ret = malloc(3 * (str_len + (((3 * str_len) / (S_QPRINT_MAXL - 9)) + 1)));
    if (ret == NULL)
        return NULL;

    d = ret;

    while (str_len--)
    {
        if (((c = *str++) == '\015') && (*str == '\012') && str_len > 0)
        {
            *d++ = '\015';
            *d++ = *str++;
            str_len--;
            lp = 0;
        }
        else
        {
            if (iscntrl(c) || (c == 0x7f) || (c & 0x80) || (c == '=') || ((c == ' ') && (*str == '\015')))
            {

                if ((((lp += 3) > S_QPRINT_MAXL) && (c <= 0x7f)) || ((c > 0x7f) && (c <= 0xdf) && ((lp + 3) > S_QPRINT_MAXL)) || ((c > 0xdf) && (c <= 0xef) && ((lp + 6) > S_QPRINT_MAXL)) || ((c > 0xef) && (c <= 0xf4) && ((lp + 9) > S_QPRINT_MAXL)))
                {
                    *d++ = '=';
                    *d++ = '\015';
                    *d++ = '\012';
                    lp = 3;
                }
                *d++ = '=';
                *d++ = hex[c >> 4];
                *d++ = hex[c & 0xf];
            }
            else
            {
                if ((++lp) > S_QPRINT_MAXL)
                {
                    *d++ = '=';
                    *d++ = '\015';
                    *d++ = '\012';
                    lp = 1;
                }
                *d++ = c;
            }
        }
    }
    *d = '\0';
    *ret_length = d - ret;

    ret = realloc(ret, *ret_length + 1);

    return ret;
}

#ifdef _TEST
// gcc -g quote_print.c -D_TEST
int main(int argc, char **argv)
{
    size_t outlen = 0;
    char *out = s_quoted_printable_encode_alloc(argv[1], strlen(argv[1]), &outlen);
    printf("%d:%s\n", outlen, out);

    char *out2 = s_quoted_printable_decode_alloc(out, outlen, &outlen);
    printf("%d:%s\n", outlen, out2);

    if (out)
        free(out);
    if (out2)
        free(out2);

    return 0;
}
#endif
