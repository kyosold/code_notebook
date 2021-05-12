#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "crc32.h"

/**
 * @brief Calculates the crc32 polynomial of a string
 * @param str The data.
 * @param str_len data length
 * @param result Returns the crc32 checksum of string
 * @param result_size result memory size
 */
void s_crc32(const char *str, size_t str_len, char *result, size_t result_size)
{
    char *p;
    int len, nr;
    uint32_t crcinit = 0;
    register uint32_t crc;

    nr = str_len;
    p = (char *)str;

    crc = crcinit ^ 0xFFFFFFFF;

    for (len = +nr; nr--; ++p)
    {
        crc = ((crc >> 8) & 0x00FFFFFF) ^ crc32tab[(crc ^ (*p)) & 0xFF];
    }
    //printf("%lu\n", crc ^ 0xFFFFFFFF);
    snprintf(result, result_size, "%lu", crc ^ 0xFFFFFFFF);
}

#ifdef _TEST
// gcc -g crc32.c -D_TEST
void main(int argc, char **argv)
{
    char res[1024] = {0};
    s_crc32(argv[1], strlen(argv[1]), res, sizeof(res));
    printf("%s\n", res);
}
#endif