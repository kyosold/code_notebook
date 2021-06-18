#include "utils.h"

/**
 * @brief   转换数字到4位的字符串
 * @param s 输出, 转换后的字符串
 * @param u 输入, 需要转换的数字
 * @return None
 */
void uint32_pack(char s[4], uint32_t u)
{
    s[0] = u & 255;
    u >>= 8;
    s[1] = u & 255;
    u >>= 8;
    s[2] = u & 255;
    s[3] = u >> 8;
}
/**
 * @brief   4位字符转成数字
 * @param s 输入，需要被转换的4位字符
 * @param u 输出，转换后的数字
 */
void uint32_unpack(char s[4], uint32_t *u)
{
    uint32_t result;

    result = (unsigned char)s[3];
    result <<= 8;
    result += (unsigned char)s[2];
    result <<= 8;
    result += (unsigned char)s[1];
    result <<= 8;
    result += (unsigned char)s[0];

    *u = result;
}

void uint32_pack_big(char s[4], uint32_t u)
{
    s[3] = u & 255;
    u >>= 8;
    s[2] = u & 255;
    u >>= 8;
    s[1] = u & 255;
    s[0] = u >> 8;
}
void uint32_unpack_big(char s[4], uint32_t *u)
{
    uint32_t result;

    result = (unsigned char)s[0];
    result <<= 8;
    result += (unsigned char)s[1];
    result <<= 8;
    result += (unsigned char)s[2];
    result <<= 8;
    result += (unsigned char)s[3];

    *u = result;
}

uint32_t scdb_hashadd(uint32_t h, unsigned char c)
{
    h += (h << 5);
    return h ^ c;
}
uint32_t scdb_hash(char *buf, unsigned int len)
{
    uint32_t h;
    h = SCDB_HASHSTART;
    while (len)
    {
        h = scdb_hashadd(h, *buf++);
        --len;
    }
    return h;
}