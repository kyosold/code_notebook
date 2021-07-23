#include <string.h>
#include <stdlib.h>
#include "base64.h"

static const char base64_table[] =
    {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
     'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
     'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '\0'};

static const char base64_pad = '=';

static const short base64_reverse_table[256] = {
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -2, -2, -2,
    -2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,
    -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2};

/**
 * @brief Encodes data with MIME base64
 * @param str The data to encode
 * @param str_len 'str' length
 * @param ret_length result length
 * @return The encoded data, as a string, to be freed with free(). Fail return NULL
 */
unsigned char *s_base64_encode_alloc(const unsigned char *str, int str_len, int *ret_length)
{
    const unsigned char *current = str;
    unsigned char *p;
    unsigned char *result;

    if ((str_len + 2) < 0 || ((str_len + 2) / 3) >= (1 << (sizeof(int) * 8 - 2)))
    {
        if (ret_length != NULL)
        {
            *ret_length = 0;
        }
        return NULL;
    }

    result = (unsigned char *)malloc((((str_len + 2) / 3) * 4) * sizeof(char) + 1);
    if (result == NULL)
    {
        return NULL;
    }
    p = result;

    while (str_len > 2)
    {
        /* keep going until we have less than 24 bits */
        *p++ = base64_table[current[0] >> 2];
        *p++ = base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
        *p++ = base64_table[((current[1] & 0x0f) << 2) + (current[2] >> 6)];
        *p++ = base64_table[current[2] & 0x3f];

        current += 3;
        str_len -= 3; // we just handle 3 octets of data
    }

    /* now deal with the tail end of things */
    if (str_len != 0)
    {
        *p++ = base64_table[current[0] >> 2];
        if (str_len > 1)
        {
            *p++ = base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
            *p++ = base64_table[(current[1] & 0x0f) << 2];
            *p++ = base64_pad;
        }
        else
        {
            *p++ = base64_table[(current[0] & 0x03) << 4];
            *p++ = base64_pad;
            *p++ = base64_pad;
        }
    }
    if (ret_length != NULL)
    {
        *ret_length = (int)(p - result);
    }
    *p = '\0';
    return result;
}

/**
 * @brief Decodes data encoded with MIME base64
 * @param str The encoded data
 * @param str_len 'str' length
 * @param ret_length result length
 * @return Returns the decoded data or false on failure. The returned data may be binary, to be freed with free(). Fail return NULL
 */
unsigned char *s_base64_decode_alloc(const unsigned char *str, int str_len, int *ret_length)
{
    const unsigned char *current = str;
    int ch, i = 0, j = 0, strict = 0, k;
    /* this sucks for threaded environments */
    unsigned char *result;

    result = (unsigned char *)malloc(str_len + 1);
    if (result == NULL)
        return NULL;

    /* run through the whole string, converting as we go */
    while ((ch = *current++) != '\0' && str_len-- > 0)
    {
        if (ch == base64_pad)
        {
            if (!current != '=' && (i % 4) == 1)
            {
                free(result);
                return NULL;
            }
            continue;
        }

        ch = base64_reverse_table[ch];
        if ((!strict && ch < 0) || ch == -1)
        {
            /* a space or some other separator character, we simply skip over */
            continue;
        }
        else if (ch == -2)
        {
            free(result);
            return NULL;
        }

        switch (i % 4)
        {
        case 0:
            result[j] = ch << 2;
            break;
        case 1:
            result[j++] |= ch >> 4;
            result[j] = (ch & 0x0f) << 4;
            break;
        case 2:
            result[j++] |= ch >> 2;
            result[j] = (ch & 0x03) << 6;
            break;
        case 3:
            result[j++] |= ch;
            break;
        }
        i++;
    }

    k = j;
    /* mop things up if we ended on a boundary */
    if (ch == base64_pad)
    {
        switch (i % 4)
        {
        case 1:
            free(result);
            return NULL;
        case 2:
            k++;
        case 3:
            result[k++] = 0;
        }
    }
    if (ret_length)
    {
        *ret_length = j;
    }
    result[j] = '\0';
    return result;
}

#ifdef _TEST
// gcc - g base64.c - D_TEST
#include <stdio.h>
int main(int argc, char **argv)
{
    int outlen = 0;
    char *out = s_base64_encode_alloc(argv[1], strlen(argv[1]), &outlen);
    printf("%d:%s\n", outlen, out);

    char *out2 = s_base64_decode_alloc(out, outlen, &outlen);
    printf("%d:%s\n", outlen, out2);

    if (out)
        free(out);
    if (out2)
        free(out2);

    return 0;
}
#endif
