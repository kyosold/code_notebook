#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "schar.h"

/**********************************************************/
#define ALIGNMENT 16 /* assuming that this alignment is enough */

/**
 * 申请n个字节内存 (实际申请的是大于n可以整除的16的字节数)
 */
char *_alloc(unsigned int n)
{
    char *x = NULL;

    // (n & (ALIGNMENT - 1): 为 n % 16。
    // 如果 n % 16 不为0，则向上加，使n可以整除16.
    // 申请的字节必须是一块一块的，必须是16字节的整数。
    n = ALIGNMENT + n - (n & (ALIGNMENT - 1));
    x = (char *)malloc(n);
    return x;
}

void _free(char *x)
{
    if (x)
        free(x);
}

/**
 * @param m x原来的空间大小
 * @param n 新的空间总大小，n 不能小于 m
 * @return 0:succ, 1:fail
 */
int _realloc(char **x, unsigned int m, unsigned int n)
{
    char *y = NULL;
    y = _alloc(n);
    if (!y)
        return SCHAR_FAIL;

    memcpy(y, *x, m);
    _free(*x);
    *x = y;

    return SCHAR_SUCC;
}

/**********************************************************/

/**
 * @brief 检查x的剩余空间是否足够放下n个字节，不够则自动扩展，
 *      如果x是空指针，则malloc给它
 * @param x 被检查的结构
 * @param n 需要存放的大小
 * @return 0:succ, 1:fail
 */
int schar_ready(schar *x, unsigned int n)
{
    unsigned int i;
    if (x->s)
    {
        i = x->size;
        n += x->len;
        if (n > i)
        {
            i = 30 + n + (n >> 3);
            if (_realloc(&x->s, x->size, i) == SCHAR_SUCC)
            {
                x->size = i;
                return SCHAR_SUCC;
            }
            return SCHAR_FAIL;
        }
        return SCHAR_SUCC;
    }
    x->len = 0;
    x->size = n;
    x->s = _alloc(x->size);
    if (x->s == NULL)
        return SCHAR_FAIL;
    return SCHAR_SUCC;
}

/**
 * @brief 复制src前n个字节到dest里，并以\0结束
 * @param dest 指向复制的目标对象
 * @param src 指向被自制的对象
 * @param n 要复制的字节数
 * @return 0:succ, 1:fail
 */
int schar_copyb(schar *dest, char *src, unsigned int n)
{
    if (schar_ready(dest, n + 1) == SCHAR_FAIL)
        return SCHAR_FAIL;
    memcpy(dest->s, src, n);
    dest->len = n;
    dest->s[n] = '\0';
    return SCHAR_SUCC;
}

/**
 * @brief 复制str到dest中，注意:str必须以'\0'结尾
 * @param dest 指向复制的目标对象
 * @param str 以'\0'结尾的字符串
 * @return 0:succ, 1:fail
 */
int schar_copys(schar *dest, char *str)
{
    return schar_copyb(dest, str, strlen(str));
}

/**
 * @brief 复制source到dest中
 * @param dest 指向复制的目标对象
 * @param source 指向被复制的源对象
 * @return 0:succ, 1:fail
 */
int schar_copy(schar *dest, schar *source)
{
    return schar_copyb(dest, source->s, source->len);
}

/**
 * @brief 追加src前n个字节到dest里，并以\0结束
 * @param dest 指向追加的目标对象
 * @param src 指向被复制的对象
 * @param n 要追加的字节数
 * @return 0:succ, 1:fail
 */
int schar_catb(schar *dest, char *src, unsigned int n)
{
    if (dest->s == NULL)
        return schar_copyb(dest, src, n);

    if (schar_ready(dest, n + 1) == SCHAR_FAIL)
        return SCHAR_FAIL;
    memcpy(dest->s + dest->len, src, n);
    dest->len += n;
    dest->s[dest->len] = '\0';
    return SCHAR_SUCC;
}

/**
 * @brief 追加str到dest中，注意:str必须以'\0'结尾
 * @param dest 指向被追加的目标对象
 * @param str 以'\0'结尾的字符串
 * @return 0:succ, 1:fail
 */
int schar_cats(schar *dest, char *str)
{
    return schar_catb(dest, str, strlen(str));
}

/**
 * @brief 追加source到dest中
 * @param dest 指向被追加的目标对象
 * @param source 指向复制的源对象
 * @return 0:succ, 1:fail
 */
int schar_cat(schar *dest, schar *source)
{
    return schar_catb(dest, source->s, source->len);
}


char *schar_strtolower(char *s, size_t len)
{
    unsigned char *c, *e;

    c = (unsigned char *)s;
    e = c + len;

    while (c < e)
    {
        *c = tolower(*c);
        c++;
    }
    return s;
}

char *schar_strtoupper(char *s, size_t len)
{
    unsigned char *c, *e;

    c = (unsigned char *)s;
    e = (unsigned char *)c + len;

    while (c < e)
    {
        *c = toupper(*c);
        c++;
    }
    return s;
}

char *_schar_memnstr(char *haystack, char *needle, int needle_len, char *end)
{
    char *p = haystack;
    char ne = needle[needle_len - 1];
    if (needle_len == 1)
        return (char *)memchr(p, *needle, (end - p));

    if (needle_len > end - haystack)
        return NULL;

    end -= needle_len;

    while (p <= end)
    {
        if ((p = (char *)memchr(p, *needle, (end - p + 1))) && ne == p[needle_len - 1])
        {
            if (!memcmp(needle, p, needle_len - 1))
                return p;
        }

        if (p == NULL)
            return NULL;

        p++;
    }

    return NULL;
}

/**
 * @brief 查找子字符串str在dest中第一次出现的位置
 * @param dest 被查找的目标
 * @param substr 查找的子字符串
 * @param substr_len 查找的子字符串的长度
 * @param offset 查找会从offset的位置开始.
 * @return 返回str第一次出现在dest中的位置,如果找不到，返回-1
 */
int schar_strpos(schar *dest, char *substr, int substr_len, unsigned int offset)
{
    if (offset > dest->len)
        return SCHAR_NOTFOUND;

    char *found = NULL;
    found = _schar_memnstr(dest->s + offset,
                           substr,
                           substr_len,
                           dest->s + dest->len);
    if (found)
        return (found - dest->s);
    else
        return SCHAR_NOTFOUND;
}

/**
 * @brief 查找子字符串str在dest中第一次出现的位置, 不区分大小写
 * @param dest 被查找的目标
 * @param substr 查找的子字符串
 * @param substr_len 查找的子字符串的长度
 * @param offset 查找会从offset的位置开始.
 * @return 返回str第一次出现在dest中的位置,如果找不到，返回-1
 */
int schar_stripos(schar *dest, char *substr, int substr_len, unsigned int offset)
{
    if (offset > dest->len)
        return SCHAR_NOTFOUND;

    char *found = NULL;
    long found_offset;
    char *haystack_dup = NULL;
    char *substr_dup = NULL;

    haystack_dup = strdup(dest->s);
    if (haystack_dup == NULL)
        return SCHAR_NOTFOUND;
    schar_strtolower(haystack_dup, dest->len);

    substr_dup = strdup(substr);
    if (substr_dup == NULL)
    {
        free(haystack_dup);
        return SCHAR_NOTFOUND;
    }
    schar_strtolower(substr_dup, substr_len);

    found = _schar_memnstr(haystack_dup + offset,
                           substr_dup,
                           substr_len,
                           haystack_dup + dest->len);
    if (found)
    {
        found_offset = found - haystack_dup;

        free(haystack_dup);
        free(substr_dup);

        return found_offset;
    }
    else
    {
        free(haystack_dup);
        free(substr_dup);

        return SCHAR_NOTFOUND;
    }
}

/**
 * @brief 查找子字符串在dest中第一次出现
 * @param dest 查找的目标
 * @param substr 查找的子字符串
 * @param substr_len 子字符串的长度
 * @param offset 查找会从offset的位置开始
 * @param before_substr 为1时，返回的是substr第一次出现之前的字符串，默认应该是0
 * @return 返回查找到的字符串，该字符串必须手动free，失败返回 NULL
 */
char *schar_strstr_alloc(schar *dest, char *substr, int substr_len, unsigned int offset, unsigned int before_substr)
{
    if (offset > dest->len)
        return NULL;

    char *ret = NULL;
    long found_offset;
    char *found = NULL;
    found = _schar_memnstr(dest->s + offset,
                           substr,
                           substr_len,
                           dest->s + dest->len);
    if (found)
    {
        found_offset = found - dest->s;
        if (before_substr)
        {
            ret = _alloc(found_offset + 1);
            if (ret == NULL)
                return NULL;
            memcpy(ret, dest->s, found_offset);
            ret[found_offset] = '\0';
        }
        else
        {
            ret = strdup(substr);
        }
        return ret;
    }
    else
        return NULL;
}

/**
 * @brief 查找子字符串在dest中第一次出现, 不区分大小写
 * @param dest 查找的目标
 * @param substr 查找的子字符串
 * @param substr_len 子字符串的长度
 * @param offset 查找会从offset的位置开始
 * @param before_substr 为1时，返回的是substr第一次出现之前的字符串，默认应该是0
 * @return 返回查找到的字符串，该字符串必须手动free，失败返回 NULL
 */
char *schar_stristr_alloc(schar *dest, char *substr, int substr_len, unsigned int offset, unsigned int before_substr)
{
    if (offset > dest->len)
        return NULL;

    char *ret = NULL;
    long found_offset;
    char *found = NULL;
    char *haystack_dup = NULL;
    char *substr_dup = NULL;

    haystack_dup = strdup(dest->s);
    if (haystack_dup == NULL)
        return NULL;
    schar_strtolower(haystack_dup, dest->len);

    substr_dup = strdup(substr);
    if (substr_dup == NULL)
    {
        free(haystack_dup);
        return NULL;
    }
    schar_strtolower(substr_dup, substr_len);

    found = _schar_memnstr(haystack_dup + offset,
                           substr_dup,
                           substr_len,
                           haystack_dup + dest->len);

    if (found)
    {
        found_offset = found - haystack_dup;

        free(haystack_dup);
        free(substr_dup);

        if (before_substr)
        {
            ret = _alloc(found_offset + 1);
            if (ret == NULL)
                return NULL;
            memcpy(ret, dest->s, found_offset);
            ret[found_offset] = '\0';
        }
        else
        {
            ret = strdup(substr);
        }
        return ret;
    }
    else
    {
        free(haystack_dup);
        free(substr_dup);
        return NULL;
    }
}

/**
 * @brief 释放x中s指向的内存
 * @param x 需要被释放的schar结构指针
 */
void schar_clean(schar *x)
{
    if (x && x->s)
    {
        _free(x->s);
        x->s = NULL;
        x->len = 0;
        x->size = 0;
    }
}

/**
 * @brief 初始并生成一个schar结构指针
 * @return schar *, 失败返回 NULL, to be free use schar_delete()
 */
schar *schar_new()
{
    schar *x = NULL;
    x = malloc(sizeof(schar));
    if (x)
    {
        x->len = 0;
        x->size = 0;
        x->s = NULL;
    }
    return x;
}

/**
 * @brief 释放销毁schar *, schar内s指向的内存也一并释放掉
 */
void schar_delete(schar *x)
{
    if (x)
    {
        schar_clean(x);

        free(x);
        x = NULL;
    }
}

#ifdef _TEST
// gcc -g schar.c -D_TEST
#include <stdio.h>
void main(int argc, char **argv)
{
    // 第一种使用方法，强烈推荐 -------------
    schar *x = schar_new();
    if (!x)
    {
        printf("alloc schar fail\n");
        return;
    }
    schar_copyb(x, "1234567890", 10);
    schar_catb(x, " | ", 3);
    schar_cats(x, "ABCDEFGfhiji");
    printf("len:%d size:%d s:\n%s\n", x->len, x->size, x->s);

    int pos = schar_stripos(x, "de", 2, 0);
    printf("[schar_stripos('de')] pos:%d str:[%s]\n", pos, x->s + pos);

    char *s = schar_strstr_alloc(x, "bc", 2, 0, 0);
    printf("[schar_strstr('bc')] str:[%s]\n", s);
    if (s)
        free(s);

    char *ss = schar_stristr_alloc(x, "bc", 2, 0, 1);
    printf("[schar_stristr('bc', 1)] str:[%s]\n", ss);
    if (ss)
        free(ss);

    schar_delete(x);
    // ----------------------------------

    printf("\n--------------------\n");

    // 第二种使用方法，不推荐 --------------
    schar str;
    str.len = 0;
    str.size = 0;
    str.s = NULL;

    schar_copyb(&str, ">>>>>>>>>>", 10);
    schar_catb(&str, "\n", 1);
    schar_cats(&str, "songjian");
    schar_catb(&str, ", nihao", 7);
    printf("len:%d size:%d s:\n%s\n", str.len, str.size, str.s);

    schar_clean(&str);
    // ----------------------------------

    printf("\n--------------------\n");
}
#endif
